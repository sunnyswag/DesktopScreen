
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ds_screen.h"
#include "ds_gpio.h"
#include "ds_spi.h"
#include "ds_data_image.h"

/*************************EPD display init function******************************************************/
//LUT download
static void lut(void)
{
	unsigned int count;
	spi_send_cmd(0x20);
	for(count=0;count<44;count++)	     
		{spi_send_data(lut_vcomDC[count]);}

	spi_send_cmd(0x21);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_ww[count]);}   
	
	spi_send_cmd(0x22);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_bw[count]);} 

	spi_send_cmd(0x23);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_wb[count]);} 

	spi_send_cmd(0x24);
	for(count=0;count<42;count++)	     
		{spi_send_data(lut_bb[count]);} 
}

static void lut1(void)
{
	unsigned int count;
	spi_send_cmd(0x20);
	for(count=0;count<44;count++)	     
		{spi_send_data(lut_vcom1[count]);}

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
	int busy;
	do
	{
		spi_send_cmd(0x71);
		busy = ds_gpio_get_screen_busy();    
	}
	while(busy);   
	// vTaskDelay(100 / portTICK_PERIOD_MS);                
}	

static void init_display(){
	vTaskDelay(10 / portTICK_PERIOD_MS);
    ds_gpio_set_screen_rst(0);		// Module reset
	vTaskDelay(10 / portTICK_PERIOD_MS);
	ds_gpio_set_screen_rst(1);

	spi_send_cmd(0x01);			//POWER SETTING 
	spi_send_data (0x03);	          
	spi_send_data (0x00);
	spi_send_data (0x2b);
	spi_send_data (0x2b);
	spi_send_data (0x03);

	spi_send_cmd(0x06);         //boost soft start
	spi_send_data (0x17);		//A
	spi_send_data (0x17);		//B
	spi_send_data (0x17);		//C       

	spi_send_cmd(0x04);  
	lcd_chkstatus();

	spi_send_cmd(0x00);			//panel setting
	spi_send_data(0xbf);		//LUT from OTP,128x296
	spi_send_data(0x0d);		//VCOM to 0V fast
		
	spi_send_cmd(0x30);			//PLL setting
    spi_send_data (0x3a);   	// 3a 100HZ   29 150Hz 39 200HZ	31 171HZ

	spi_send_cmd(0x61);			//resolution setting
	spi_send_data (0x98);        	 
	spi_send_data (0x00);		
	spi_send_data (0x98);
		
	spi_send_cmd(0x82);			//vcom_DC setting  	
    spi_send_data (0x28);	

	spi_send_cmd(0X50);			//VCOM AND DATA INTERVAL SETTING			
	spi_send_data(0x97);		//WBmode:VBDF 17|D7 VBDW 97 VBDB 57		WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
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

//图片全刷-全白函数
static void ds_screen_display_white(void){
	unsigned int i;
	spi_send_cmd(0x10);
	for(i=0;i<2888;i++){
		spi_send_data(0x00);  
	}
	spi_send_cmd(0x13);
	for(i=0;i<2888;i++){
		spi_send_data(0xff);  
	}  	 		
}


//图片全刷-数据函数
void ds_screen_full_display_data(const uint8_t *data){
	unsigned int i;
	spi_send_cmd(0x10);
	for(i=0;i<2888;i++)	     
	{
		spi_send_data(0x00);  
	}  
	spi_send_cmd(0x13);
	for(i=0;i<2888;i++)	     
	{
		spi_send_data(data[i]);  
	}  
}

//全刷 不带数据
void ds_screen_full_display(void pic_display(void)){
	init_display();
	pic_display(); 				//picture
    lut(); 						//Power settings
	spi_send_cmd(0x12);			//DISPLAY REFRESH 	
	lcd_chkstatus();
	deep_sleep();

}

//全刷 带数据
void ds_screen_full_display_bydata(void display_func(const uint8_t *data),const uint8_t *data){
	init_display();
	display_func(data); 				//picture
    lut(); 						//Power settings
	spi_send_cmd(0x12);			//DISPLAY REFRESH 	
	lcd_chkstatus();
	deep_sleep();
}

//局部刷 不带数据
void ds_screen_partial_display(unsigned int x_start,unsigned int x_end,unsigned int y_start,unsigned int y_end ,
								void partial_old(void),
								void partial_new(void))
{
	init_display();
	spi_send_cmd(0x82);			//vcom_DC setting  	
    spi_send_data (0x08);	
	spi_send_cmd(0X50);
	spi_send_data(0x47);		
	lut1();
	spi_send_cmd(0x91);		//This command makes the display enter partial mode
	spi_send_cmd(0x90);		//resolution setting
	spi_send_data (x_start);   //x-start     
	spi_send_data (x_end-1);	 //x-end	

	spi_send_data (y_start/256);
	spi_send_data (y_start%256);   //y-start    
		
	spi_send_data (y_end/256);		
	spi_send_data (y_end%256-1);  //y-end
	spi_send_data (0x28);	

	spi_send_cmd(0x10);	       //writes Old data to SRAM for programming
	partial_old();
	spi_send_cmd(0x13);		   //writes New data to SRAM.
	partial_new();
    	
	spi_send_cmd(0x12);		 //DISPLAY REFRESH 		 
	lcd_chkstatus();
	deep_sleep();
	vTaskDelay(200 / portTICK_PERIOD_MS);  
}

//局部刷 带数据
void ds_screen_partial_display_bydata(unsigned int x_start,unsigned int x_end,unsigned int y_start,unsigned int y_end ,
										void partial_old(const uint8_t *data),const uint8_t *old_data,
										void partial_new(const uint8_t *data),const uint8_t *new_data) 
{
	init_display();
	spi_send_cmd(0x82);			//vcom_DC setting  	
    spi_send_data (0x08);	
	spi_send_cmd(0X50);
	spi_send_data(0x47);		
	lut1();
	spi_send_cmd(0x91);		//This command makes the display enter partial mode
	spi_send_cmd(0x90);		//resolution setting
	spi_send_data (x_start);   //x-start     
	spi_send_data (x_end-1);	 //x-end	

	spi_send_data (y_start/256);
	spi_send_data (y_start%256);   //y-start    
		
	spi_send_data (y_end/256);		
	spi_send_data (y_end%256-1);  //y-end
	spi_send_data (0x28);	

	spi_send_cmd(0x10);	       //writes Old data to SRAM for programming
	partial_old(old_data);
	spi_send_cmd(0x13);		   //writes New data to SRAM.
	partial_new(new_data);
    	
	spi_send_cmd(0x12);		 //DISPLAY REFRESH 		 
	lcd_chkstatus();
	deep_sleep();
	vTaskDelay(200 / portTICK_PERIOD_MS);   
}

uint8_t partial_data[152][19];
uint8_t partial_data_array[2888];

void ds_screen_partial_data_init(){
	for(int index = 0;index < 152 ;index ++){
		for(int yindex = 0;yindex < 19 ;yindex ++){
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
	//x_data_location=0 x_size=5  
	//90 126
	// printf("x_data_location %d x_size%d\n",x_data_location,x_size);
	// printf("ystart %d y_end%d\n",y_start,y_end);
	int status = 0;
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
	for(i=0;i<2888;i++){
		spi_send_data(0x00);  
	}
	spi_send_cmd(0x13);
	for(i=0;i<2888;i++){
		spi_send_data(partial_data_array[i]);  
	}  	 		
}

void ds_screen_partial_data_copy(){
	int data_index = 0;
	for(int index = 0;index < 152 ;index ++){
		for(int yindex = 0;yindex < 19 ;yindex ++){
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

//初始化
void ds_screen_init(){
	ds_screen_full_display(ds_screen_display_white);
}

//清屏为白色
void ds_screen_clean_white(){
	ds_screen_init();
	vTaskDelay(2000 / portTICK_PERIOD_MS);      
}





