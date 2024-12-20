
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ds_screen.h"
#include "ds_gpio.h"
#include "ds_spi.h"
#include "ds_data_image.h"

// Detection busy
static void lcd_chkstatus(void)
{
    int count = 0;
    unsigned char busy;
    while (1)
    {
        busy = ds_gpio_get_screen_busy();
        busy = (busy & 0x01);
        //=1 BUSY
        if (busy == 0)
            break;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        count++;
        if (count >= 1000)
        {
            printf("---------------time out ---\n");
            break;
        }
    }
}

static void init_display()
{
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ds_gpio_set_screen_rst(0); // Module reset
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ds_gpio_set_screen_rst(1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    lcd_chkstatus();
    spi_send_cmd(0x12); // SWRESET
    lcd_chkstatus();

    spi_send_cmd(0x01); // Driver output control
    spi_send_data(0xC7);
    spi_send_data(0x00);
    spi_send_data(0x01);

    spi_send_cmd(0x11); // data entry mode
    spi_send_data(0x01);

    spi_send_cmd(0x44); // set Ram-X address start/end position
    spi_send_data(0x00);
    spi_send_data(0x18); // 0x0C-->(18+1)*8=200

    spi_send_cmd(0x45);  // set Ram-Y address start/end position
    spi_send_data(0xC7); // 0xC7-->(199+1)=200
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_cmd(0x3C); // BorderWavefrom
    spi_send_data(0x05);

    spi_send_cmd(0x18); // Read built-in temperature sensor
    spi_send_data(0x80);

    spi_send_cmd(0x4E); // set RAM x address count to 0;
    spi_send_data(0x00);
    spi_send_cmd(0x4F); // set RAM y address count to 0X199;
    spi_send_data(0xC7);
    spi_send_data(0x00);

    vTaskDelay(100 / portTICK_PERIOD_MS);
    lcd_chkstatus();
}

/////////////////////////////Enter deep sleep mode////////////////////////
void deep_sleep(void) // Enter deep sleep mode
{
    spi_send_cmd(0x10); // enter deep sleep
    spi_send_data(0x01);
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void refresh(void)
{
    spi_send_cmd(0x22); // Display Update Control
    spi_send_data(0xF7);
    spi_send_cmd(0x20); // Activate Display Update Sequence
    lcd_chkstatus();
}

void refresh_part(void)
{
    spi_send_cmd(0x22); // Display Update Control
    spi_send_data(0xFF);
    spi_send_cmd(0x20); // Activate Display Update Sequence
    lcd_chkstatus();
}

// 图片全刷-全白函数
static void ds_screen_display_white(void)
{
    unsigned int i, k;
    spi_send_cmd(0x24); // write RAM for black(0)/white (1)
    for (k = 0; k < 250; k++)
    {
        for (i = 0; i < 25; i++)
        {
            spi_send_data(0xff);
        }
    }
}

// 图片全刷-数据函数
void ds_screen_full_display_data(const uint8_t *data)
{

    unsigned int i;
    spi_send_cmd(0x24); // write RAM for black(0)/white (1)
    for (i = 0; i < 5000; i++)
    {
        spi_send_data(*data);
        data++;
    }
}

// 全刷 不带数据
void ds_screen_full_display(void pic_display(void))
{
    init_display();
    pic_display(); // picture
    refresh();     // EPD_refresh
    deep_sleep();
}

// 全刷 带数据
void ds_screen_full_display_bydata(void display_func(const uint8_t *data), const uint8_t *data)
{
    init_display();
    display_func(data); // picture
    refresh();          // EPD_refresh
    deep_sleep();
}

// 局部刷 不带数据
void ds_screen_partial_display(unsigned int x_start, unsigned int y_start, void partial_new(void), unsigned int PART_COLUMN, unsigned int PART_LINE)
{
    unsigned int i;
    unsigned int x_end, y_start1, y_start2, y_end1, y_end2;
    x_start = x_start / 8;
    x_end = x_start + PART_LINE / 8 - 1;

    y_start1 = 0;
    y_start2 = 200 - y_start;
    if (y_start >= 256)
    {
        y_start1 = y_start2 / 256;
        y_start2 = y_start2 % 256;
    }
    y_end1 = 0;
    y_end2 = y_start2 + PART_COLUMN - 1;
    if (y_end2 >= 256)
    {
        y_end1 = y_end2 / 256;
        y_end2 = y_end2 % 256;
    }

    // Add hardware reset to prevent background color change
    ds_gpio_set_screen_rst(0); // Module reset
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ds_gpio_set_screen_rst(1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Lock the border to prevent flashing
    spi_send_cmd(0x3C); // BorderWavefrom,
    spi_send_data(0x80);

    spi_send_cmd(0x44);      // set RAM x address start/end, in page 35
    spi_send_data(x_start);  // RAM x address start at 00h;
    spi_send_data(x_end);    // RAM x address end at 0fh(15+1)*8->128
    spi_send_cmd(0x45);      // set RAM y address start/end, in page 35
    spi_send_data(y_start2); // RAM y address start at 0127h;
    spi_send_data(y_start1); // RAM y address start at 0127h;
    spi_send_data(y_end2);   // RAM y address end at 00h;
    spi_send_data(y_end1);   // ????=0

    spi_send_cmd(0x4E); // set RAM x address count to 0;
    spi_send_data(x_start);
    spi_send_cmd(0x4F); // set RAM y address count to 0X127;
    spi_send_data(y_start2);
    spi_send_data(y_start1);

    spi_send_cmd(0x24); // Write Black and White image to RAM
    partial_new();

    refresh_part();
    deep_sleep();
}

// 局部刷 带数据
void ds_screen_partial_display_bydata(unsigned int x_start, unsigned int y_start,
                                      void partial_new(const uint8_t *data), const uint8_t *new_data,
                                      unsigned int PART_COLUMN, unsigned int PART_LINE)
{
    printf("x_start=%d x_end=%d y_start=%d y_end=%d\n", x_start, y_start, PART_COLUMN, PART_LINE);

    unsigned int i;
    unsigned int x_end, y_start1, y_start2, y_end1, y_end2;
    x_start = x_start / 8;
    x_end = x_start + PART_LINE / 8 - 1;

    y_start1 = 0;
    y_start2 = 200 - y_start;
    if (y_start >= 256)
    {
        y_start1 = y_start2 / 256;
        y_start2 = y_start2 % 256;
    }
    y_end1 = 0;
    y_end2 = y_start2 + PART_COLUMN - 1;
    if (y_end2 >= 256)
    {
        y_end1 = y_end2 / 256;
        y_end2 = y_end2 % 256;
    }
    // Add hardware reset to prevent background color change
    ds_gpio_set_screen_rst(0); // Module reset
    vTaskDelay(10 / portTICK_PERIOD_MS);
    ds_gpio_set_screen_rst(1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Lock the border to prevent flashing
    spi_send_cmd(0x3C); // BorderWavefrom,
    spi_send_data(0x80);

    spi_send_cmd(0x44);      // set RAM x address start/end, in page 35
    spi_send_data(x_start);  // RAM x address start at 00h;
    spi_send_data(x_end);    // RAM x address end at 0fh(15+1)*8->128
    spi_send_cmd(0x45);      // set RAM y address start/end, in page 35
    spi_send_data(y_start2); // RAM y address start at 0127h;
    spi_send_data(y_start1); // RAM y address start at 0127h;
    spi_send_data(y_end2);   // RAM y address end at 00h;
    spi_send_data(y_end1);   // ????=0

    spi_send_cmd(0x4E); // set RAM x address count to 0;
    spi_send_data(x_start);
    spi_send_cmd(0x4F); // set RAM y address count to 0X127;
    spi_send_data(y_start2);
    spi_send_data(y_start1);

    spi_send_cmd(0x24); // Write Black and White image to RAM
    partial_new(new_data);
}

uint8_t partial_data[200][25];
uint8_t partial_data_array[5000];

void ds_screen_partial_data_init()
{
    for (int index = 0; index < 200; index++)
    {
        for (int yindex = 0; yindex < 25; yindex++)
        {
            partial_data[index][yindex] = 0xff;
        }
    }
}

void ds_screen_partial_data_add(unsigned int x_start, unsigned int x_end, unsigned int y_start, unsigned int y_end, const uint8_t *data)
{
    uint8_t x_len = x_end - x_start;
    // uint8_t y_len = y_end - y_start;
    uint8_t x_data_location = x_start / 8;
    uint8_t x_size = x_len / 8;
    int data_index = 0;
    // int move = x_start % 8;
    if (x_start % 8 != 0)
    {
        x_data_location++;
    }
    if (x_len % 8 != 0)
    {
        x_size++;
    }
    for (int x_index = y_start; x_index < y_end; x_index++)
    {
        for (int y_index = x_data_location; y_index < (x_data_location + x_size); y_index++)
        {
            partial_data[x_index][y_index] = (~data[data_index]);
            data_index++;
        }
    }
}

// 图片全刷-全白函数
static void ds_screen_display_data(void)
{
    unsigned int i;
    spi_send_cmd(0x24);
    for (i = 0; i < 5000; i++)
    {
        spi_send_data(partial_data_array[i]);
    }
    spi_send_cmd(0x26);
    for (i = 0; i < 5000; i++)
    {
        spi_send_data(partial_data_array[i]);
    }
}

// 局刷数据-复制
void ds_screen_partial_data_copy()
{
    int data_index = 0;
    for (int index = 0; index < 200; index++)
    {
        for (int yindex = 0; yindex < 25; yindex++)
        {
            partial_data_array[data_index] = partial_data[index][yindex];
            data_index++;
        }
    }
    ds_screen_full_display(ds_screen_display_data);
}

// 接口初始化
void init_screen_interface()
{
    ds_screen_gpio_init();
    screen_spi_init();
}

// 清屏为白色
void ds_screen_clean_white()
{
    ds_screen_init();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

// 初始化
void ds_screen_init()
{
    ds_screen_full_display(ds_screen_display_white); // EPD_sleep
}
