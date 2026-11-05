// 性能探针：周期性输出堆、任务栈、任务 CPU 时间和关键事件。
// 所有输出行以 "PERF " 开头，由 tools/perf/parse_probe_log.py 解析；
// 改动输出格式时要同步修改解析脚本，并把 PERF_PROBE_VERSION 加一。
// 串口在主机侧（WSL + usbipd）偶尔丢字节，所以每组输出都带校验值，解析时对不上的整组丢弃。
// 只用 32 位整数格式化，开启 newlib nano 格式化后也能正常输出。
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "perf_probe_priv.h"

#define PERF_PROBE_VERSION 3
#define PERF_SAMPLE_PERIOD_MS 5000
#define PERF_POLL_PERIOD_MS 100
#define PERF_PROBE_STACK 4096
#define PERF_MAX_TASKS 40
#define PERF_MAX_SPANS 64
// 2023-11 之后的时间戳才算对过时，用来记录 time_valid 事件
#define PERF_TIME_VALID_EPOCH 1700000000

typedef struct
{
    char name[configMAX_TASK_NAME_LEN];
    uint32_t stack_bytes;
} perf_task_info_t;

typedef struct
{
    const char *name;
    int64_t start_us;
    int64_t dur_us;
    int32_t arg;
} perf_span_t;

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static perf_task_info_t s_tasks[PERF_MAX_TASKS];
static int s_task_count;
static perf_span_t *s_spans;
static unsigned s_span_head;
static unsigned s_span_tail;
static unsigned s_span_dropped;
static TaskStatus_t *s_status;
static int64_t s_app_main_us;
static size_t s_dynamic_bytes;

static unsigned long to_ms(int64_t us) { return (unsigned long)(us / 1000); }

void perf_probe_task_created(const char *name, uint32_t stack_bytes)
{
    portENTER_CRITICAL(&s_lock);
    if (s_task_count < PERF_MAX_TASKS)
    {
        strlcpy(s_tasks[s_task_count].name, name ? name : "", sizeof(s_tasks[0].name));
        s_tasks[s_task_count].stack_bytes = stack_bytes;
        s_task_count++;
    }
    portEXIT_CRITICAL(&s_lock);
}

// 同名任务取最后创建的那个；FreeRTOS 内部创建的任务（IDLE、Tmr Svc）查不到，返回 0
static uint32_t lookup_stack_bytes(const char *name)
{
    for (int i = s_task_count - 1; i >= 0; i--)
    {
        if (strncmp(s_tasks[i].name, name, sizeof(s_tasks[0].name)) == 0)
        {
            return s_tasks[i].stack_bytes;
        }
    }
    return 0;
}

void perf_probe_span(const char *name, int64_t start_us, int64_t dur_us, int32_t arg)
{
    if (s_spans == NULL)
    {
        return;
    }
    portENTER_CRITICAL_SAFE(&s_lock);
    if (s_span_head - s_span_tail < PERF_MAX_SPANS)
    {
        perf_span_t *span = &s_spans[s_span_head % PERF_MAX_SPANS];
        span->name = name;
        span->start_us = start_us;
        span->dur_us = dur_us;
        span->arg = arg;
        s_span_head++;
    }
    else
    {
        s_span_dropped++;
    }
    portEXIT_CRITICAL_SAFE(&s_lock);
}

static void drain_spans(void)
{
    for (;;)
    {
        perf_span_t span;
        bool has_span = false;
        portENTER_CRITICAL(&s_lock);
        if (s_span_tail != s_span_head)
        {
            span = s_spans[s_span_tail % PERF_MAX_SPANS];
            s_span_tail++;
            has_span = true;
        }
        portEXIT_CRITICAL(&s_lock);
        if (!has_span)
        {
            return;
        }
        unsigned long t_ms = to_ms(span.start_us);
        unsigned long dur_us = (unsigned long)span.dur_us;
        printf("PERF span t_ms=%lu dur_us=%lu arg=%ld name=%s chk=%lu\n", t_ms, dur_us,
               (long)span.arg, span.name, t_ms + dur_us);
    }
}

// arg 带上 STA 的 IPv4 地址（网络字节序），供 capture_serial.py 从电脑发起配网页请求；摘要里不输出
static void on_got_ip(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    const ip_event_got_ip_t *event = data;
    perf_probe_span("got_ip", esp_timer_get_time(), 0, (int32_t)event->ip_info.ip.addr);
}

static void on_ap_sta_join(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    perf_probe_span("ap_sta_join", esp_timer_get_time(), 0, 0);
}

static bool hook_events(void)
{
    if (esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_got_ip, NULL) != ESP_OK)
    {
        return false;
    }
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, on_ap_sta_join, NULL);
    return true;
}

static void sample(void)
{
    configRUN_TIME_COUNTER_TYPE rt_total = 0;
    UBaseType_t count = uxTaskGetSystemState(s_status, PERF_MAX_TASKS, &rt_total);
    unsigned long t_ms = to_ms(esp_timer_get_time());

    unsigned heap[6] = {
        heap_caps_get_free_size(MALLOC_CAP_8BIT),
        heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_total_size(MALLOC_CAP_8BIT),
    };
    printf("PERF heap t_ms=%lu free8=%u min8=%u largest8=%u int_free=%u int_min=%u total8=%u "
           "spans_dropped=%u chk=%u\n",
           t_ms, heap[0], heap[1], heap[2], heap[3], heap[4], heap[5], s_span_dropped,
           heap[0] + heap[1] + heap[2] + heap[3] + heap[4] + heap[5]);
    printf("PERF tasks t_ms=%lu rt_total=%lu count=%u\n", t_ms, (unsigned long)rt_total,
           (unsigned)count);
    unsigned long rt_sum = 0;
    unsigned long hwm_sum = 0;
    for (UBaseType_t i = 0; i < count; i++)
    {
        rt_sum += (unsigned long)s_status[i].ulRunTimeCounter;
        hwm_sum += (unsigned long)s_status[i].usStackHighWaterMark;
#if (configTASKLIST_INCLUDE_COREID == 1)
        int core = taskVALID_CORE_ID(s_status[i].xCoreID) ? (int)s_status[i].xCoreID : -1;
#else
        int core = -2;
#endif
        printf("PERF task t_ms=%lu rt=%lu hwm=%lu stack=%lu prio=%u core=%d name=%s\n", t_ms,
               (unsigned long)s_status[i].ulRunTimeCounter,
               (unsigned long)s_status[i].usStackHighWaterMark,
               (unsigned long)lookup_stack_bytes(s_status[i].pcTaskName),
               (unsigned)s_status[i].uxCurrentPriority, core, s_status[i].pcTaskName);
    }
    // 32 位回绕求和，解析脚本按同样方式校验
    printf("PERF tasks_end t_ms=%lu rt_sum=%lu hwm_sum=%lu\n", t_ms, rt_sum, hwm_sum);
}

static void perf_probe_task(void *arg)
{
#ifdef PERF_PROBE_AUTO
    const int auto_scenario = 1;
#else
    const int auto_scenario = 0;
#endif
    printf("PERF hello version=%d sample_ms=%d cpu_mhz=%d static_bytes=%u dynamic_bytes=%u "
           "app_main_ms=%lu auto=%d\n",
           PERF_PROBE_VERSION, PERF_SAMPLE_PERIOD_MS, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
           (unsigned)(sizeof(s_tasks) + sizeof(s_lock)), (unsigned)s_dynamic_bytes,
           to_ms(s_app_main_us), auto_scenario);

    bool events_hooked = false;
    bool time_valid = false;
    int64_t next_sample_us = 0;
    for (;;)
    {
        int64_t now = esp_timer_get_time();
        // 默认事件循环由 app_main 创建，创建之前注册会返回 ESP_ERR_INVALID_STATE，所以反复尝试
        if (!events_hooked)
        {
            events_hooked = hook_events();
        }
        if (!time_valid && time(NULL) > PERF_TIME_VALID_EPOCH)
        {
            time_valid = true;
            perf_probe_span("time_valid", now, 0, 0);
        }
#ifdef PERF_PROBE_AUTO
        perf_scenario_poll(now);
#endif
        drain_spans();
        if (now >= next_sample_us)
        {
            sample();
            next_sample_us = now + PERF_SAMPLE_PERIOD_MS * 1000LL;
        }
        vTaskDelay(pdMS_TO_TICKS(PERF_POLL_PERIOD_MS));
    }
}

void perf_probe_start(void)
{
    s_app_main_us = esp_timer_get_time();
    // 先把所有内存分配完再创建任务，这样开销能算准（任务一创建就可能开始打印）
    size_t before = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    s_spans = calloc(PERF_MAX_SPANS, sizeof(perf_span_t));
    s_status = calloc(PERF_MAX_TASKS, sizeof(TaskStatus_t));
    StackType_t *stack = malloc(PERF_PROBE_STACK);
    StaticTask_t *tcb = malloc(sizeof(StaticTask_t));
    s_dynamic_bytes = before - heap_caps_get_free_size(MALLOC_CAP_8BIT);
    if (s_spans == NULL || s_status == NULL || stack == NULL || tcb == NULL)
    {
        printf("PERF error out_of_memory\n");
        return;
    }
    perf_probe_task_created("perf_probe", PERF_PROBE_STACK);
    // 固定在 core1：串口输出会忙等 UART，解析时从 core1 负载里扣掉探针自己的时间
    xTaskCreateStaticPinnedToCore(perf_probe_task, "perf_probe", PERF_PROBE_STACK, NULL, 1, stack,
                                  tcb, 1);
}
