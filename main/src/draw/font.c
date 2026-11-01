

#include "esp_partition.h"
#include <string.h>

typedef struct
{
    uint16_t min;
    uint16_t max;
    uint8_t bpp;
    uint8_t reserved[3];
} x_header_t;

typedef struct
{
    uint8_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t ofs_x;
    int8_t ofs_y;
    uint8_t r;
} glyph_dsc_t;

#define FLASH_ADDR_DATA 0x40
#define FLASH_ADDR_NVS 0x01

static x_header_t __g_xbf_hd = {
    .min = 0x0020,
    .max = 0x9fa0,
    .bpp = 4,
};

static uint8_t __g_font_buf[300]; // 如bin文件存在SPI FLASH可使用此buff

const esp_partition_t *partition_res = NULL;

static uint8_t *user_font_getdata(int offset, int size)
{
    // 如字模保存在SPI FLASH, SPIFLASH_Read(__g_font_buf,offset,size);
    // 如字模已加载到SDRAM,直接返回偏移地址即可如:return (uint8_t*)(sdram_fontddr+offset);
    static uint8_t first_in = 1;
    if (first_in == 1)
    {
        // 这个函数第一个参数是我们分区表的第四行的，第二列的参数，第二个是第三列的值
        partition_res = esp_partition_find_first(FLASH_ADDR_DATA, FLASH_ADDR_NVS, NULL);
        first_in = 0;
        if (partition_res == NULL)
        {
            printf("Failed to open file for reading\n");
            return NULL;
        }
        else
        {
            printf("Successfully open file for reading\n");
        }
    }
    esp_err_t res = esp_partition_read(partition_res, offset, __g_font_buf, size); // 读取数据
    if (res != ESP_OK)
    {
        printf("Failed to reading %x \n", offset);
    }
    return __g_font_buf;
}

static uint32_t user_font_get_glyph_dsc(uint32_t unicode_letter, glyph_dsc_t *gdsc)
{
    if (unicode_letter > __g_xbf_hd.max || unicode_letter < __g_xbf_hd.min)
    {
        return 0;
    }
    uint32_t unicode_offset = sizeof(x_header_t) + (unicode_letter - __g_xbf_hd.min) * 4;
    uint32_t *p_pos = (uint32_t *)user_font_getdata(unicode_offset, 4);
    if (p_pos[0] != 0)
    {
        uint32_t pos = p_pos[0];
        glyph_dsc_t *mdsc = (glyph_dsc_t *)user_font_getdata(pos, sizeof(glyph_dsc_t));
        memcpy(gdsc, mdsc, sizeof(glyph_dsc_t));
        return pos;
    }
    return 0;
}

static const uint8_t *user_font_get_bitmap(uint32_t pos, int size)
{
    return user_font_getdata(pos + sizeof(glyph_dsc_t), size);
}

int ds_get_bitmap(uint32_t letter, uint8_t *bitmap_buf, uint8_t *box_w, uint8_t *box_h,
                  uint8_t *offset_x, uint8_t *offset_y)
{
    glyph_dsc_t gdsc;
    uint32_t pos = user_font_get_glyph_dsc(letter, &gdsc);
    if (pos != 0)
    {
        int size = gdsc.box_w * gdsc.box_h * __g_xbf_hd.bpp / 8;
        *box_w = gdsc.box_w;
        *box_h = gdsc.box_h;
        *offset_x = gdsc.ofs_x;
        *offset_y = gdsc.ofs_y;
        user_font_get_bitmap(pos, size);
        memcpy(bitmap_buf, __g_font_buf, size);
        return size;
    }
    return 0;
}

// SimHei,,-1
// 字模高度：18
// XBF字体,外部bin文件

// /**
//  * Decode an UTF-8 character from a string.
//  * @param txt pointer to '\0' terminated string
//  * @param i start byte index in 'txt' where to start.
//  *          After call it will point to the next UTF-8 char in 'txt'.
//  *          NULL to use txt[0] as index
//  * @return the decoded Unicode character or 0 on invalid UTF-8 code
//  */
//     /* Unicode to UTF-8
//      * 00000000 00000000 00000000 0xxxxxxx -> 0xxxxxxx
//      * 00000000 00000000 00000yyy yyxxxxxx -> 110yyyyy 10xxxxxx
//      * 00000000 00000000 zzzzyyyy yyxxxxxx -> 1110zzzz 10yyyyyy 10xxxxxx
//      * 00000000 000wwwzz zzzzyyyy yyxxxxxx -> 11110www 10zzzzzz 10yyyyyy 10xxxxxx
//      * */

//     /*Dummy 'i' pointer is required*/

//     /*Normal ASCII*/
//     /*Real UTF-8 decode*/
//         /*2 bytes UTF-8 code*/
//         /*3 bytes UTF-8 code*/
//         /*4 bytes UTF-8 code*/
