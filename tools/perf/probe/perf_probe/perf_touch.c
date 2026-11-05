// 触摸模拟：自动剧本用它模拟用户点按和滑动，走被测代码完整的触摸链路：
//   GPIO 中断处理函数 → gpio_task（读电平、手势判定、日志）→ 定时器任务按 10 ms 读 FT6336 坐标 → UI 事件。
// 做法是包装三个函数，不驱动任何硬件引脚（触摸芯片的 INT 脚是它自己在驱动，不能从 ESP32 这边拉）：
//   - gpio_isr_handler_add/remove：记下 gpio.c 给触摸 INT 脚注册的中断处理函数，模拟边沿时直接调用它；
//   - gpio_get_level：模拟期间 INT 脚返回模拟电平（0 = 按下），其他引脚照常读硬件；
//   - i2c_master_read_slave：模拟期间按 FT6336 寄存器格式返回模拟触点，不访问 I2C。
// 测不到的部分：真实的 GPIO 中断上下文和 I2C 总线传输时间。
// 依赖被测代码的引脚号、i2c_master_read_slave() 接口和 FT6336 寄存器地址（以 6f09703 为准）。
#include <stdbool.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "perf_probe_priv.h"

#define PERF_TOUCH_INT_GPIO 4 // gpio.c 的 GPIO_INPUT_IO_0
#define FT_REG_GEST_ID 0x01
#define FT_REG_TD_STATUS 0x02
#define FT_REG_P1 0x03
#define TAP_HOLD_MS 100
#define SWIPE_MOVE_MS 200
#define SWIPE_HOLD_MS 250
#define RELEASE_SETTLE_MS 100

esp_err_t __real_gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args);
esp_err_t __real_gpio_isr_handler_remove(gpio_num_t gpio_num);
int __real_gpio_get_level(gpio_num_t gpio_num);
extern esp_err_t __real_i2c_master_read_slave(uint8_t reg, uint8_t *data, size_t size)
    __attribute__((weak));

static gpio_isr_t s_isr;
static void *s_isr_arg;
static volatile bool s_active;
static volatile int s_level = 1;
static volatile int64_t s_t0;
static volatile int s_x0, s_y0, s_x1, s_y1, s_move_us;

esp_err_t __wrap_gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args)
{
    if (gpio_num == PERF_TOUCH_INT_GPIO)
    {
        s_isr_arg = args;
        s_isr = isr_handler;
    }
    return __real_gpio_isr_handler_add(gpio_num, isr_handler, args);
}

esp_err_t __wrap_gpio_isr_handler_remove(gpio_num_t gpio_num)
{
    if (gpio_num == PERF_TOUCH_INT_GPIO)
    {
        s_isr = NULL;
    }
    return __real_gpio_isr_handler_remove(gpio_num);
}

// 可能被放在 IRAM 里的代码调用，所以包装函数也放 IRAM
int IRAM_ATTR __wrap_gpio_get_level(gpio_num_t gpio_num)
{
    if (s_active && gpio_num == PERF_TOUCH_INT_GPIO)
    {
        return s_level;
    }
    return __real_gpio_get_level(gpio_num);
}

esp_err_t __wrap_i2c_master_read_slave(uint8_t reg, uint8_t *data, size_t size)
{
    if (!s_active)
    {
        return __real_i2c_master_read_slave(reg, data, size);
    }
    memset(data, 0, size);
    if (reg == FT_REG_TD_STATUS && size >= 1)
    {
        data[0] = s_level == 0 ? 1 : 0; // 按下期间报告 1 个触点
    }
    else if (reg == FT_REG_P1 && size >= 4)
    {
        int64_t elapsed = esp_timer_get_time() - s_t0;
        int x = s_x0;
        int y = s_y0;
        if (s_move_us > 0)
        {
            int64_t t = elapsed < s_move_us ? elapsed : s_move_us;
            x = s_x0 + (int)((s_x1 - s_x0) * t / s_move_us);
            y = s_y0 + (int)((s_y1 - s_y0) * t / s_move_us);
        }
        data[0] = (x >> 8) & 0x0F;
        data[1] = x & 0xFF;
        data[2] = (y >> 8) & 0x0F;
        data[3] = y & 0xFF;
    }
    // FT_REG_GEST_ID 以及其他寄存器返回 0
    return ESP_OK;
}

bool perf_touch_available(void) { return s_isr != NULL && __real_i2c_master_read_slave != NULL; }

// 坐标是触摸芯片的原始值（0~199），被测代码会再缩放到 152×152
static void gesture(int x0, int y0, int x1, int y1, int move_ms, int hold_ms)
{
    s_x0 = x0;
    s_y0 = y0;
    s_x1 = x1;
    s_y1 = y1;
    s_move_us = move_ms * 1000;
    s_level = 0;
    s_t0 = esp_timer_get_time();
    s_active = true;
    s_isr(s_isr_arg); // 按下边沿
    vTaskDelay(pdMS_TO_TICKS(hold_ms));
    s_level = 1;
    s_isr(s_isr_arg); // 抬起边沿
    vTaskDelay(pdMS_TO_TICKS(RELEASE_SETTLE_MS));
    s_active = false;
}

void perf_touch_tap(int x, int y) { gesture(x, y, x, y, 0, TAP_HOLD_MS); }

void perf_touch_swipe(int x0, int y0, int x1, int y1)
{
    gesture(x0, y0, x1, y1, SWIPE_MOVE_MS, SWIPE_HOLD_MS);
}
