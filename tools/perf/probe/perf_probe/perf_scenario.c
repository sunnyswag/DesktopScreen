// 自动测试剧本：按 tools/perf/scenario.txt 的四个阶段，由探针自己模拟用户操作（device_probe.sh 默认开启）。
//   - 阶段 2 翻页：用 perf_touch.c 模拟点按和右滑，走完整的触摸链路；触摸链路不可用时（接口被重构掉了）
//     退回直接调用 ds_ui_page_manage_send_event()，这时 gpio_task 和坐标解析不会被执行。
//   - 阶段 3 配网页：默认由电脑通过局域网请求设备的 httpd（capture_serial.py 收到阶段 3 的事件后发起），
//     数据真实经过 WiFi；编译时定义 PERF_PROBE_HTTP_LOOPBACK 则改为设备自己通过 127.0.0.1 请求。
//     两种方式都没有手机关联 AP、DHCP 分配地址的过程。
// 依赖被测代码的 TP_ACTION_E 取值和主页的象限布局（以 6f09703 为准），UI 改动后要同步修改。
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_timer.h"
#include "lwip/sockets.h"

#include "perf_probe_priv.h"

// system_data.h 里的 TP_ACTION_E
#define TP_ACTION_SHORT 1
#define TP_ACTION_MOVE_RIGHT 4
#define HTTP_TIMEOUT_S 3

extern void ds_ui_page_manage_send_event(int key, uint8_t touch_x, uint8_t touch_y)
    __attribute__((weak));

typedef enum
{
    STEP_PHASE,
    STEP_TAP,
    STEP_SWIPE_RIGHT,
    STEP_HTTP,
} step_kind_t;

typedef struct
{
    uint32_t at_ms;
    step_kind_t kind;
    int arg;
    uint8_t x;
    uint8_t y;
    const char *path;
} step_t;

// 时间以 esp_timer 启动为起点，和 scenario.txt 的阶段划分一致。坐标是触摸芯片原始值（0~199）。
// 主页按象限进入页面：左上时钟、右上天气、右下番茄钟、左下单词卡；右滑返回主页。每次全刷约 2.5 秒。
static const step_t s_steps[] = {
    {.at_ms = 0, .kind = STEP_PHASE, .arg = 1},
    {.at_ms = 60000, .kind = STEP_PHASE, .arg = 2},
    {.at_ms = 60000, .kind = STEP_TAP, .x = 40, .y = 40},
    {.at_ms = 68000, .kind = STEP_SWIPE_RIGHT},
    {.at_ms = 76000, .kind = STEP_TAP, .x = 160, .y = 40},
    {.at_ms = 84000, .kind = STEP_SWIPE_RIGHT},
    {.at_ms = 92000, .kind = STEP_TAP, .x = 160, .y = 160},
    {.at_ms = 100000, .kind = STEP_SWIPE_RIGHT},
    {.at_ms = 108000, .kind = STEP_TAP, .x = 40, .y = 160},
    {.at_ms = 116000, .kind = STEP_SWIPE_RIGHT},
    {.at_ms = 120000, .kind = STEP_PHASE, .arg = 3},
    {.at_ms = 125000, .kind = STEP_HTTP, .path = "/"},
    {.at_ms = 126000, .kind = STEP_HTTP, .path = "/favicon.ico"},
    {.at_ms = 140000, .kind = STEP_HTTP, .path = "/"},
    {.at_ms = 141000, .kind = STEP_HTTP, .path = "/favicon.ico"},
    {.at_ms = 155000, .kind = STEP_HTTP, .path = "/"},
    {.at_ms = 156000, .kind = STEP_HTTP, .path = "/favicon.ico"},
    {.at_ms = 180000, .kind = STEP_PHASE, .arg = 4},
};

static unsigned s_next_step;

// 被测代码里滑动事件的 action 字段没有初始化（issue #10）。退回直接发 UI 事件时，先把下面要用到的栈清零，
// 保证每次结果一样；走触摸模拟时这个字段在 gpio_task 的栈上，和真实触摸一样不可控。
static void __attribute__((noinline)) scrub_stack(void)
{
    volatile uint8_t buf[256];
    memset((void *)buf, 0, sizeof(buf));
}

static void touch_step(const step_t *step, int64_t now)
{
    bool swipe = step->kind == STEP_SWIPE_RIGHT;
    if (perf_touch_available())
    {
        if (swipe)
        {
            perf_touch_swipe(30, 100, 170, 100);
        }
        else
        {
            perf_touch_tap(step->x, step->y);
        }
        perf_probe_span(swipe ? "auto_swipe" : "auto_tap", now, esp_timer_get_time() - now, 1);
    }
    else if (ds_ui_page_manage_send_event != NULL)
    {
        scrub_stack();
        ds_ui_page_manage_send_event(swipe ? TP_ACTION_MOVE_RIGHT : TP_ACTION_SHORT,
                                     step->x * 152 / 200, step->y * 152 / 200);
        perf_probe_span(swipe ? "auto_swipe" : "auto_tap", now, 0, 0);
    }
    else
    {
        perf_probe_span("auto_unsupported", now, 0, step->kind);
    }
}

#ifdef PERF_PROBE_HTTP_LOOPBACK
static char s_buf[256];

// 返回收到的字节数，失败返回负数。收到完整响应（Content-Length 或分块结束标记）就返回，不等连接关闭。
static int http_get(const char *path)
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
    {
        return -1;
    }
    struct timeval tv = {.tv_sec = HTTP_TIMEOUT_S};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(80),
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
    };
    int total = -2;
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
    {
        int len = snprintf(s_buf, sizeof(s_buf),
                           "GET %s HTTP/1.1\r\nHost: 192.168.4.1\r\nConnection: close\r\n\r\n", path);
        total = -3;
        if (send(sock, s_buf, len, 0) == len)
        {
            total = 0;
            int expected = -1;
            char tail[6] = {0};
            int n;
            while ((n = recv(sock, s_buf, sizeof(s_buf) - 1, 0)) > 0)
            {
                s_buf[n] = '\0';
                if (total == 0)
                {
                    const char *cl = strstr(s_buf, "Content-Length: ");
                    const char *body = strstr(s_buf, "\r\n\r\n");
                    if (cl && body)
                    {
                        expected = (int)(body + 4 - s_buf) + atoi(cl + 16);
                    }
                }
                total += n;
                // 记住最后 5 个字节，用来识别分块传输的结束标记 "0\r\n\r\n"
                int keep = n >= 5 ? 0 : 5 - n;
                memmove(tail, tail + 5 - keep, keep);
                memcpy(tail + keep, s_buf + n - (5 - keep), 5 - keep);
                if ((expected >= 0 && total >= expected) || memcmp(tail, "0\r\n\r\n", 5) == 0)
                {
                    break;
                }
            }
        }
    }
    close(sock);
    return total;
}
#endif

void perf_scenario_poll(int64_t now_us)
{
    while (s_next_step < sizeof(s_steps) / sizeof(s_steps[0]) &&
           now_us >= (int64_t)s_steps[s_next_step].at_ms * 1000)
    {
        const step_t *step = &s_steps[s_next_step++];
        switch (step->kind)
        {
        case STEP_PHASE:
            perf_probe_span("auto_phase", now_us, 0, step->arg);
            break;
        case STEP_TAP:
        case STEP_SWIPE_RIGHT:
            touch_step(step, now_us);
            break;
        case STEP_HTTP:
#ifdef PERF_PROBE_HTTP_LOOPBACK
        {
            int64_t t0 = esp_timer_get_time();
            int bytes = http_get(step->path);
            perf_probe_span("auto_http", t0, esp_timer_get_time() - t0, bytes);
        }
#endif
        break;
        }
        now_us = esp_timer_get_time();
    }
}
