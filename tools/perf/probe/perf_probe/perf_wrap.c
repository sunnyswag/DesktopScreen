// 链接器 --wrap 的包装函数，符号列表在 CMakeLists.txt 的 PERF_PROBE_WRAPS 里。
// 被测代码里的函数统一声明成 weak：某个函数被改名或删掉后，探针仍能链接，只是对应事件不再出现。
// 参数类型只要求和被测代码 ABI 一致（枚举按 int、句柄按指针传），所以不依赖 main 组件的头文件。
#include <stdint.h>

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "perf_probe_priv.h"

void __real_app_main(void);
BaseType_t __real_xTaskCreatePinnedToCore(TaskFunction_t code, const char *const name,
                                          const uint32_t stack_bytes, void *const param,
                                          UBaseType_t prio, TaskHandle_t *const handle,
                                          const BaseType_t core);
BaseType_t __real_xTaskCreatePinnedToCoreWithCaps(TaskFunction_t code, const char *const name,
                                                  const configSTACK_DEPTH_TYPE stack_bytes,
                                                  void *const param, UBaseType_t prio,
                                                  TaskHandle_t *const handle, const BaseType_t core,
                                                  UBaseType_t caps);
extern void __real_ds_screen_full_display(void (*pic_display)(void)) __attribute__((weak));
extern void __real_ds_screen_full_display_bydata(const uint8_t *data) __attribute__((weak));
extern void __real_ds_screen_refresh(int mode) __attribute__((weak));
extern void __real_ds_screen_partial_data_copy(void) __attribute__((weak));
extern esp_err_t __real_esp_http_client_perform(void *client) __attribute__((weak));
extern esp_err_t __real_esp_light_sleep_start(void) __attribute__((weak));

void __wrap_app_main(void)
{
    perf_probe_start();
    __real_app_main();
    perf_probe_span("app_main_return", esp_timer_get_time(), 0, 0);
}

BaseType_t __wrap_xTaskCreatePinnedToCore(TaskFunction_t code, const char *const name,
                                          const uint32_t stack_bytes, void *const param,
                                          UBaseType_t prio, TaskHandle_t *const handle,
                                          const BaseType_t core)
{
    perf_probe_task_created(name, stack_bytes);
    return __real_xTaskCreatePinnedToCore(code, name, stack_bytes, param, prio, handle, core);
}

// httpd 等组件用这个接口创建任务
BaseType_t __wrap_xTaskCreatePinnedToCoreWithCaps(TaskFunction_t code, const char *const name,
                                                  const configSTACK_DEPTH_TYPE stack_bytes,
                                                  void *const param, UBaseType_t prio,
                                                  TaskHandle_t *const handle, const BaseType_t core,
                                                  UBaseType_t caps)
{
    perf_probe_task_created(name, stack_bytes);
    return __real_xTaskCreatePinnedToCoreWithCaps(code, name, stack_bytes, param, prio, handle, core,
                                                  caps);
}

// 墨水屏全刷：包含上电初始化、写 RAM、等 BUSY、进屏幕深睡
void __wrap_ds_screen_full_display(void (*pic_display)(void))
{
    int64_t t0 = esp_timer_get_time();
    __real_ds_screen_full_display(pic_display);
    perf_probe_span("screen_full", t0, esp_timer_get_time() - t0, 0);
}

void __wrap_ds_screen_full_display_bydata(const uint8_t *data)
{
    int64_t t0 = esp_timer_get_time();
    __real_ds_screen_full_display_bydata(data);
    perf_probe_span("screen_full_bydata", t0, esp_timer_get_time() - t0, 0);
}

// 时间页、番茄页的整页刷新：把拼好的局刷缓冲写两遍 RAM 后全刷
void __wrap_ds_screen_partial_data_copy(void)
{
    int64_t t0 = esp_timer_get_time();
    __real_ds_screen_partial_data_copy();
    perf_probe_span("screen_full_partial", t0, esp_timer_get_time() - t0, 0);
}

// 只统计跨文件调用（目前是时钟局刷）；screen.c 内部调用不经过包装函数
void __wrap_ds_screen_refresh(int mode)
{
    int64_t t0 = esp_timer_get_time();
    __real_ds_screen_refresh(mode);
    perf_probe_span("screen_refresh", t0, esp_timer_get_time() - t0, mode);
}

esp_err_t __wrap_esp_http_client_perform(void *client)
{
    int64_t t0 = esp_timer_get_time();
    esp_err_t err = __real_esp_http_client_perform(client);
    perf_probe_span("http_perform", t0, esp_timer_get_time() - t0, err);
    return err;
}

esp_err_t __wrap_esp_light_sleep_start(void)
{
    int64_t t0 = esp_timer_get_time();
    esp_err_t err = __real_esp_light_sleep_start();
    perf_probe_span("light_sleep", t0, esp_timer_get_time() - t0, err);
    return err;
}
