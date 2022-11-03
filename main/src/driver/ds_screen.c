
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ds_screen.h"
#include "ds_gpio.h"
#include "ds_spi.h"
#include "ds_data_image.h"
#include "ds_data_page.h"

/////////////////////////////////////partial screen update LUT///////////////////////////////////////////
const unsigned char lut_vcomDC1[] ={	0x01,	0x04,	0x04,	0x03,	0x01,	0x01,	0x01,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x01,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	}; 
								
const unsigned char lut_ww1[] ={	0x01,	0x04,	0x04,	0x03,	0x01,	0x01,	0x01,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	};			
								
const unsigned char lut_bw1[] ={	0x01,	0x84,	0x84,	0x83,	0x01,	0x01,	0x01,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	};
								
const unsigned char lut_wb1[] ={	0x01,	0x44,	0x44,	0x43,	0x01,	0x01,	0x01,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	};
									
const unsigned char lut_bb1[] ={	0x01,	0x04,	0x04,	0x03,	0x01,	0x01,	0x01,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	
	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	};

void lut1(void)
{
	unsigned int count;
	spi_send_cmd(0x20);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_vcomDC1[count]);}

	spi_send_cmd(0x21);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_ww1[count]);}   
	
	spi_send_cmd(0x22);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_bw1[count]);} 

	spi_send_cmd(0x23);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_wb1[count]);} 

	spi_send_cmd(0x24);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_bb1[count]);}   
}

//Detection busy
static void lcd_chkstatus(void)
{
	int count = 0;
	unsigned char busy;
	do
	{
		spi_send_cmd(0x71);
		busy = ds_gpio_get_screen_busy();
		busy =!(busy & 0x01);        
		vTaskDelay(10 / portTICK_PERIOD_MS);  
		count ++;
		if(count >= 1000){
			printf("---------------time out ---\n");
			break;                  
		}
	}
	while(busy);   
	// vTaskDelay(200 / portTICK_PERIOD_MS);                    
}		

static void init_display(){
	vTaskDelay(10 / portTICK_PERIOD_MS);
    ds_gpio_set_screen_rst(0);		// Module reset
	vTaskDelay(10 / portTICK_PERIOD_MS);
	ds_gpio_set_screen_rst(1);
	vTaskDelay(100 / portTICK_PERIOD_MS);

	spi_send_cmd(0x00);        			//panel setting
    spi_send_data (0xDf);
	spi_send_data (0x0e);	
	
	spi_send_cmd(0x4D);        			//FITIinternal code
    spi_send_data (0x55);

	spi_send_cmd(0xaa);        			
    spi_send_data (0x0f);
		
	spi_send_cmd(0xE9);        			
    spi_send_data (0x02);

	spi_send_cmd(0xb6);        			
    spi_send_data (0x11);
		
	spi_send_cmd(0xF3);        			
    spi_send_data (0x0a);     

	spi_send_cmd(0x61);			//resolution setting
    spi_send_data (0xc8);
	spi_send_data (0x00);		
    spi_send_data (0xc8);    
		
	spi_send_cmd(0x60);        	 //Tcon setting		
    spi_send_data (0x00);
		
	spi_send_cmd(0X50);						
	spi_send_data(0x97);//
		
	spi_send_cmd(0XE3);						
	spi_send_data(0x00);
		
	spi_send_cmd(0x04);//Power on
	vTaskDelay(100 / portTICK_PERIOD_MS);
	lcd_chkstatus();
}

/////////////////////////////Enter deep sleep mode////////////////////////
static void deep_sleep(void) //Enter deep sleep mode
{
	spi_send_cmd(0X50);
	spi_send_data(0xf7);	
	spi_send_cmd(0X02);  	//power off
  	lcd_chkstatus();
	spi_send_cmd(0X07);  	//deep sleep
	spi_send_data(0xA5);
}

void refresh(void)
{
	spi_send_cmd(0x12);					//DISPLAY REFRESH 	
	vTaskDelay(100 / portTICK_PERIOD_MS);	        //!!!The delay here is necessary, 200uS at least!!!     
	lcd_chkstatus();
}

//图片全刷-全白函数
static void ds_screen_display_white(void){
	unsigned int i,j;	
	for(j=0;j<2;j++){
		spi_send_cmd(0x10);
		for(i=0;i<5000;i++)	     {
			spi_send_data(0x00);  
		}  
		vTaskDelay(2 / portTICK_PERIOD_MS);
		spi_send_cmd(0x13);
		for(i=0;i<5000;i++)	     {
			spi_send_data(0xff);  
		}  
		vTaskDelay(2 / portTICK_PERIOD_MS);
		spi_send_cmd(0x12);			//DISPLAY REFRESH 	
		vTaskDelay(200 / portTICK_PERIOD_MS);	    //!!!The delay here is necessary, 200uS at least!!!     
		lcd_chkstatus();	
	}       //waiting for the electronic paper IC to release the idle signal	 		
}


//图片全刷-数据函数
void ds_screen_full_display_data(const uint8_t *data){
	unsigned int i;
	spi_send_cmd(0x10);
	for(i=0;i<5000;i++)	     
	{
		spi_send_data(0xff);  
	}  
	vTaskDelay(2 / portTICK_PERIOD_MS);
	spi_send_cmd(0x13);
	for(i=0;i<5000;i++)	     
	{
		spi_send_data(data[i]);  
	}  
	vTaskDelay(2 / portTICK_PERIOD_MS);
}

//全刷 不带数据
void ds_screen_full_display(void pic_display(void)){
	init_display();
	pic_display(); 				//picture
	refresh();														//EPD_refresh		
	deep_sleep();
}

//全刷 带数据
void ds_screen_full_display_bydata(void display_func(const uint8_t *data),const uint8_t *data){
	init_display();
	display_func(data); 				//picture
	refresh();														//EPD_refresh		
	deep_sleep();	
}

//局部刷 不带数据
void ds_screen_partial_display(unsigned int x_start,unsigned int x_end,unsigned int y_start,unsigned int y_end ,
								void partial_old(void),
								void partial_new(void))
{
	unsigned datas,i;
	init_display();
	spi_send_cmd(0x00);        			//panel setting
    spi_send_data (0xff);
	spi_send_data (0x0e);		    
	lut1();

	spi_send_cmd(0x91);		//This command makes the display enter partial mode
	spi_send_cmd(0x90);		//resolution setting
	spi_send_data (x_start);   //x-start     
	spi_send_data (x_end-1);	 //x-end	
	spi_send_data (0);	 //x Reserved	

	spi_send_data (y_start);   //y-start    
    spi_send_data (0);	 //y Reserved		
	spi_send_data (y_end);	 //y-end	
	spi_send_data (0x01);	
    datas=(x_end-x_start)*(y_end-y_start)/8;
		
	spi_send_cmd(0x10);	       //writes Old data to SRAM for programming
	partial_old();
	spi_send_cmd(0x13);				 //writes New data to SRAM.
	partial_new();	
	refresh();
	deep_sleep();
}

//局部刷 带数据
void ds_screen_partial_display_bydata(unsigned int x_start,unsigned int x_end,unsigned int y_start,unsigned int y_end ,
										void partial_old(const uint8_t *data),const uint8_t *old_data,
										void partial_new(const uint8_t *data),const uint8_t *new_data) 
{ 	
    printf("update h = %d %d %d %d\n",x_start,x_end,y_start,y_end);

	unsigned datas,i;
	init_display();
	spi_send_cmd(0x00);        			//panel setting
    spi_send_data (0xff);
	spi_send_data (0x0e);		    
	lut1();

	spi_send_cmd(0x91);		//This command makes the display enter partial mode
	spi_send_cmd(0x90);		//resolution setting
	spi_send_data (x_start);   //x-start     
	spi_send_data (x_end-1);	 //x-end	
	spi_send_data (0);	 //x Reserved	

	spi_send_data (y_start);   //y-start    
    spi_send_data (0);	 //y Reserved		
	spi_send_data (y_end);	 //y-end	
	spi_send_data (0x01);	
    datas=(x_end-x_start)*(y_end-y_start)/8;
		
	spi_send_cmd(0x10);	       //writes Old data to SRAM for programming
	partial_old(old_data);
	spi_send_cmd(0x13);				 //writes New data to SRAM.
	partial_new(new_data);
	refresh();
	deep_sleep();
}

uint8_t partial_data[200][25];
uint8_t partial_data_array[5000];

void ds_screen_partial_data_init(){
	for(int index = 0;index < 200 ;index ++){
		for(int yindex = 0;yindex < 25 ;yindex ++){
			partial_data[index][yindex] = 0xff;
		}
	}
}

void ds_screen_partial_data_add(unsigned int x_start,unsigned int x_end,unsigned int y_start,unsigned int y_end ,const uint8_t *data){
	uint8_t x_len = x_end - x_start;
	// uint8_t y_len = y_end - y_start;
	uint8_t x_data_location = x_start/8;   
	uint8_t x_size = x_len/8;   
	int data_index = 0;
	// int move = x_start % 8;
	if(x_start % 8 != 0){
		x_data_location ++;
	}
	if(x_len % 8 != 0){
		x_size ++;
	}
	for(int x_index = y_start ;x_index < y_end;x_index ++){
		for(int y_index = x_data_location ;y_index < (x_data_location+x_size);y_index ++){
			partial_data[x_index][y_index] = (~data[data_index]);
			data_index++;
		}
	}
}

//图片全刷-全白函数
static void ds_screen_display_data(void){
	unsigned int i;
	spi_send_cmd(0x10);
	for(i=0;i<5000;i++){
		spi_send_data(0xff);  
	}
	spi_send_cmd(0x13);
	for(i=0;i<5000;i++){
		spi_send_data(partial_data_array[i]);  
	}  	 		
}

//局刷数据-复制
void ds_screen_partial_data_copy(){
	int data_index = 0;
	for(int index = 0;index < 200 ;index ++){
		for(int yindex = 0;yindex < 25 ;yindex ++){
			partial_data_array[data_index] = partial_data[index][yindex];
			data_index ++;
		}
	}
	ds_screen_full_display(ds_screen_display_data);
}

//接口初始化
void init_screen_interface(){
    ds_screen_gpio_init();
    screen_spi_init();
}

//清屏为白色
void ds_screen_clean_white(){
	ds_screen_init();
	vTaskDelay(2000 / portTICK_PERIOD_MS);      
}

//初始化
void ds_screen_init(){
	ds_screen_full_display(ds_screen_display_white);														//EPD_sleep
}

