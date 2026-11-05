#pragma once

#include <stdbool.h>
#include <stdint.h>

// 记录一段耗时（包装函数里调用）。只写环形缓冲，不打印，避免改变被测任务的栈水位。
void perf_probe_span(const char *name, int64_t start_us, int64_t dur_us, int32_t arg);

// 记录任务创建时的栈大小（字节），采样时和栈水位一起输出。
void perf_probe_task_created(const char *name, uint32_t stack_bytes);

// 在 app_main 之前启动采样任务。
void perf_probe_start(void);

// 自动剧本（perf_scenario.c），定义了 PERF_PROBE_AUTO 时由采样任务每 100 ms 调用一次
void perf_scenario_poll(int64_t now_us);

// 触摸模拟（perf_touch.c），坐标是触摸芯片原始值；调用期间会阻塞几百毫秒
bool perf_touch_available(void);
void perf_touch_tap(int x, int y);
void perf_touch_swipe(int x0, int y0, int x1, int y1);
