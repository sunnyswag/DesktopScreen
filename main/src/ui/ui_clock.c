#include "ui_clock.h"
#include "screen.h"
#include "spi.h"
#include "data_num.h"
#include "data_icon.h"

enum
{
    DIGIT_WIDTH = 32,
    DIGIT_HEIGHT = 48,
    COLON_WIDTH = 8,
};

static void add_clock_number(unsigned int x, unsigned int y, uint8_t number)
{
    ds_screen_partial_data_add(x, x + DIGIT_WIDTH, y, y + DIGIT_HEIGHT,
                               gImage_num_size48[number / 10]);
    ds_screen_partial_data_add(x + DIGIT_WIDTH, x + DIGIT_WIDTH * 2, y, y + DIGIT_HEIGHT,
                               gImage_num_size48[number % 10]);
}

void ds_ui_clock_add_time(unsigned int x, unsigned int y, uint8_t hour, uint8_t minute)
{
    add_clock_number(x, y, hour);
    ds_screen_partial_data_add(x + DIGIT_WIDTH * 2, x + DIGIT_WIDTH * 2 + COLON_WIDTH, y,
                               y + DIGIT_HEIGHT, gImage_time_symbol);
    add_clock_number(x + DIGIT_WIDTH * 2 + COLON_WIDTH, y, minute);
}

static void send_clock_digit(const uint8_t *data)
{
    for (unsigned int i = 0; i < DIGIT_WIDTH * DIGIT_HEIGHT / 8; ++i)
    {
        spi_send_byte(~data[i], SPI_BYTE_DATA);
    }
}

static void update_clock_number(unsigned int x, unsigned int y, uint8_t number)
{
    ds_screen_partial_display_bydata(x, y, send_clock_digit, gImage_num_size48[number / 10],
                                     DIGIT_HEIGHT, DIGIT_WIDTH);
    ds_screen_partial_display_bydata(x + DIGIT_WIDTH, y, send_clock_digit,
                                     gImage_num_size48[number % 10], DIGIT_HEIGHT, DIGIT_WIDTH);
}

void ds_ui_clock_update_time(unsigned int x, unsigned int y, uint8_t hour, uint8_t minute)
{
    update_clock_number(x, y, hour);
    update_clock_number(x + DIGIT_WIDTH * 2 + COLON_WIDTH, y, minute);
    ds_screen_refresh(DS_SCREEN_REFRESH_PARTIAL);
    deep_sleep();
}
