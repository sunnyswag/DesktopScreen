#include "paint.h"
#include "screen.h"
#include "spi.h"

PAINT Paint;

uint8_t *ds_paint_create_image(uint16_t width, uint16_t height)
{
    size_t size = ((width + 7) / 8) * height;
    uint8_t *image = malloc(size);
    if (image == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        return NULL;
    }
    Paint_NewImage(image, width, height, 0, WHITE);
    Paint_SelectImage(image);
    Paint_Clear(WHITE);
    return image;
}

ROTATE_IMAGE Epd_Rotate = ROTATE_90;

uint8_t UTF8toUnicode(uint8_t *ch, uint16_t *_unicode)
{
    uint8_t *p = NULL ,n = 0;
    uint32_t e = 0;
    p = ch;
    if(1)//p == NULL
    {
            if(*p >= 0xfc)
            {
                    /*6:<11111100>*/
                    e  = (p[0] & 0x01) << 30;
                    e |= (p[1] & 0x3f) << 24;
                    e |= (p[2] & 0x3f) << 18;
                    e |= (p[3] & 0x3f) << 12;
                    e |= (p[4] & 0x3f) << 6;
                    e |= (p[5] & 0x3f);
                    n = 6;
            }
            else if(*p >= 0xf8)
            {
                    /*5:<11111000>*/
                    e = (p[0] & 0x03) << 24;
                    e |= (p[1] & 0x3f) << 18;
                    e |= (p[2] & 0x3f) << 12;
                    e |= (p[3] & 0x3f) << 6;
                    e |= (p[4] & 0x3f);
                    n = 5;
            }
            else if(*p >= 0xf0)
            {
                    /*4:<11110000>*/
                    e = (p[0] & 0x07) << 18;
                    e |= (p[1] & 0x3f) << 12;
                    e |= (p[2] & 0x3f) << 6;
                    e |= (p[3] & 0x3f);
                    n = 4;
            }
            else if(*p >= 0xe0)
            {
                    /*3:<11100000>*/
                    e = (p[0] & 0x0f) << 12;
                    e |= (p[1] & 0x3f) << 6;
                    e |= (p[2] & 0x3f);
                    n = 3;
            }
            else if(*p >= 0xc0)
            {
                    /*2:<11000000>*/
                    e = (p[0] & 0x1f) << 6;
                    e |= (p[1] & 0x3f);
                    n = 2;
            }
            else
            {
                    e = p[0];           
                    n = 1;
            }
            *_unicode = e;
    }
    return n;
}

/******************************************************************************
function: Create _Image
parameter:
    _image   :   Pointer to the _image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint_NewImage(UBYTE *_image, UWORD Width, UWORD Height, UWORD Rotate, UWORD Color)
{
    Paint._Image = NULL;
    Paint._Image = _image;

    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;    
    Paint.Scale = 2;
    
    Paint.WidthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
    Paint.HeightByte = Height;    
//    printf("WidthByte = %d, HeightByte = %d\r\n", Paint.WidthByte, Paint.HeightByte);
//    printf(" EPD_WIDTH / 8 = %d\r\n",  122 / 8);
   
    Paint.Rotate = Rotate;
    Paint.Mirror = MIRROR_NONE;
    
    if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}

/******************************************************************************
function: Select _Image
parameter:
    _image : Pointer to the _image cache
******************************************************************************/
void Paint_SelectImage(UBYTE *_image)
{
    Paint._Image = _image;
}


/******************************************************************************
function: Draw Pixels
parameter:
    Xpoint : At point X
    Ypoint : At point Y
    Color  : Painted colors
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    if(Xpoint > Paint.Width || Ypoint > Paint.Height){
        printf("Exceeding display boundaries Xpoint*Ypoint=%d*%d\r\n",Xpoint,Ypoint);
        return;
    }      
    UWORD X, Y;

    switch(Paint.Rotate) {
    case 0:
        X = Xpoint;
        Y = Ypoint;  
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;
    default:
        return;
    }
    
    switch(Paint.Mirror) {
    case MIRROR_NONE:
        break;
    case MIRROR_HORIZONTAL:
        X = Paint.WidthMemory - X - 1;
        break;
    case MIRROR_VERTICAL:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case MIRROR_ORIGIN:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    if(X > Paint.WidthMemory || Y > Paint.HeightMemory){
        printf("Exceeding display boundaries X*Y=%d*%d\r\n",X,Y);
        return;
    }
    
    if(Paint.Scale == 2){
        UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
        UBYTE Rdata = Paint._Image[Addr];
        if(Color == BLACK)
            Paint._Image[Addr] = Rdata & ~(0x80 >> (X % 8));
        else
            Paint._Image[Addr] = Rdata | (0x80 >> (X % 8));
    }else if(Paint.Scale == 4){
        UDOUBLE Addr = X / 4 + Y * Paint.WidthByte;
        Color = Color % 4;//Guaranteed color scale is 4  --- 0~3
        UBYTE Rdata = Paint._Image[Addr];
        
        Rdata = Rdata & (~(0xC0 >> ((X % 4)*2)));
        Paint._Image[Addr] = Rdata | ((Color << 6) >> ((X % 4)*2));
    }
}

/******************************************************************************
function: Clear the color of the picture
parameter:
    Color : Painted colors
******************************************************************************/
void Paint_Clear(UWORD Color)
{
    for (UWORD Y = 0; Y < Paint.HeightByte; Y++) {
        for (UWORD X = 0; X < Paint.WidthByte; X++ ) {//8 pixel =  1 byte
            UDOUBLE Addr = X + Y*Paint.WidthByte;
            Paint._Image[Addr] = Color;
        }
    }
}

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : Y starting point
    Xend   : x end point
    Yend   : y end point
    Color  : Painted colors
******************************************************************************/

/******************************************************************************
function: Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint    : The Xpoint coordinate of the point
    Ypoint    : The Ypoint coordinate of the point
    Color   : Painted color
    Dot_Pixel : point size
    Dot_Style : point Style
******************************************************************************/

/******************************************************************************
function: Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment
    Line_width : Line width
    Line_Style: Solid and dotted lines
******************************************************************************/

/******************************************************************************
function: Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting Xpoint point coordinates
    Ystart ：Rectangular  Starting Xpoint point coordinates
    Xend   ：Rectangular  End point Xpoint coordinate
    Yend   ：Rectangular  End point Ypoint coordinate
    Color  ：The color of the Rectangular segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the rectangle
******************************************************************************/

/******************************************************************************
function: Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  ：Center X coordinate
    Y_Center  ：Center Y coordinate
    Radius    ：circle Radius
    Color     ：The color of the ：circle segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the Circle
******************************************************************************/

/******************************************************************************
function: Display the string
parameter:
    Xstart  ：X coordinate
    Ystart  ：Y coordinate
    pString ：The first address of the Chinese string and English
              string to be displayed
    Font    ：A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString,
                        UWORD Color_Foreground, UWORD Color_Background)
{
    const char* pstr = pString;
    int i;
    uint8_t  bitmap[300];
    i = 0;
    uint16_t char_cn_un = 0;
    
    //当前中文字数
    uint16_t cn_data_num = 0;
    uint16_t cn_box_w = 0;
    //当前英文字数
    uint16_t en_data_num = 0;
    uint16_t en_box_w = 0;
    while (*(pstr+i) != '\0')
    {
        memset(&bitmap,0,sizeof(bitmap));
        UTF8toUnicode((uint8_t*)pstr+i,&char_cn_un);
        uint8_t box_w = 0,box_h = 0;
        uint8_t offset_x = 0,offset_y = 0;
        int size = ds_get_bitmap(char_cn_un,bitmap,&box_w,&box_h,&offset_x,&offset_y);
            
        uint8_t page_len = box_h;  //box_h
        uint8_t colnum_len = box_w/2; //box_w/2
        printf("page_len %d  colnum_len %d\n",page_len,colnum_len); 
        for(uint8_t Page = 0; Page < page_len; Page ++) //0-127
        { 
            for (uint8_t Column = 0; Column < colnum_len; Column++) //0-1
            {
                uint8_t tmp = bitmap[Page*colnum_len + Column]; 
                // printf("%x ",tmp);  
                for(int index = 0;index < 8 ;index += 4){
                    if (((tmp >> index) & 0x0f) >= 1){
                        Paint_SetPixel( Xstart + 2*Column + (1- index/4) + (en_box_w + cn_box_w)+(en_data_num+cn_data_num)*3 + offset_x,Ystart+ Page -offset_y -page_len    , Color_Background);
                    }else{
                        Paint_SetPixel(  Xstart + 2*Column + (1- index/4) + (en_box_w + cn_box_w)+(en_data_num+cn_data_num)*3 + offset_x,Ystart+ Page - offset_y -page_len  , Color_Foreground);
                    }
                }
            }
            // printf("\n"); 
        }

        if (*(pstr+i) >= 128){
            //中文4byte
            i+=3;
            cn_data_num ++;
            cn_box_w += box_w;
        }else{
            i++;
            en_data_num ++;
            en_box_w += box_w;
        }
    }
}

void Paint_DrawString_piture(UWORD Xstart, UWORD Ystart, const unsigned char *pbuf ,int plen,int pxsize,
                        UWORD Color_Foreground, UWORD Color_Background)
{
    uint8_t width = (pxsize%8 == 0) ? (pxsize/8) : (pxsize/8+1);
    uint8_t height = plen / width;
    for(int y = 0;y < height; y ++){
        for(int x = 0;x < width ;x ++){
            for(int index = 0;index < 8;index ++){
                if(((pbuf[y*width + x]>>(7-index)) & 0x01) == 1){
                    Paint_SetPixel( Xstart + x*8 + index ,Ystart+ y , Color_Background);
                }else{
                    Paint_SetPixel(  Xstart + x*8 + index,Ystart+ y , Color_Foreground);
                }
            }
        }
    }
}

//图片全刷

unsigned char gImage_last_page[5000];


void ds_paint_image_new(){
    unsigned int i;
    spi_send_byte(0x24, SPI_BYTE_COMMAND); 
    for(i=0;i<5000;i++){
        spi_send_byte(Paint._Image[i], SPI_BYTE_DATA);  
	}  	 	
}

void ds_paint_image_copy(){
    memcpy(gImage_last_page,Paint._Image,5000);
}
