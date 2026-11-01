
#ifndef _DS_PAINT_H_
#define _DS_PAINT_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iconv.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <string.h>  
#include <sys/stat.h> 
#include "ds_font.h"


#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

typedef struct {
    UBYTE *_Image;
    UWORD Width;
    UWORD Height;
    UWORD WidthMemory;
    UWORD HeightMemory;
    UWORD Color;
    UWORD Rotate;
    UWORD Mirror;
    UWORD WidthByte;
    UWORD HeightByte;
    UWORD Scale;
} PAINT;



#define WHITE          0xFF
#define BLACK          0x00


typedef enum {
    ROTATE_0  = 0x00,
    ROTATE_90 = 0x01,
    ROTATE_180 = 0x02,
    ROTATE_270 = 0x03,
} ROTATE_IMAGE;



/**
 * The size of the point
**/
  
/**
 * Point size fill style
**/

/**
 * Line style, solid or dashed
**/

typedef enum {
    MIRROR_NONE  = 0x00,
    MIRROR_HORIZONTAL = 0x01,
    MIRROR_VERTICAL = 0x02,
    MIRROR_ORIGIN = 0x03,
} MIRROR_IMAGE;
#define MIRROR_IMAGE_DFT MIRROR_NONE

/**
 * Whether the graphic is filled
**/


void Paint_NewImage(UBYTE *_image, UWORD Width, UWORD Height, UWORD Rotate, UWORD Color);
void Paint_SelectImage(UBYTE *_image);
void Paint_Clear(UWORD Color);
void Paint_DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString,
                        UWORD Color_Foreground, UWORD Color_Background);
void Paint_DrawString_piture(UWORD Xstart, UWORD Ystart,const unsigned char *pbuf ,int plen,int pxsize,
                        UWORD Color_Foreground, UWORD Color_Background);
// The caller owns the selected image and must free it after drawing.
uint8_t *ds_paint_create_image(uint16_t width, uint16_t height);
void ds_paint_image_new(void);
void ds_paint_image_copy(void);
#endif