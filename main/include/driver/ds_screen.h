#ifndef _DS_SCREEN_H_
#define _DS_SCREEN_H_


void init_screen_interface(void);


void ds_screen_partial_display_bydata(unsigned int x_start,unsigned int y_start,
                                        void partial_new(const uint8_t *data),const uint8_t *new_data,
                                        unsigned int PART_COLUMN,unsigned int PART_LINE);

void ds_screen_full_display(void pic_display(void));
void ds_screen_full_display_bydata(const uint8_t *data);


void ds_screen_partial_data_init();
void ds_screen_partial_data_add(unsigned int x_start,unsigned int x_end,unsigned int y_start,unsigned int y_end ,const uint8_t *data);
void ds_screen_partial_data_copy();

typedef enum
{
    DS_SCREEN_REFRESH_FULL = 0xF7,
    DS_SCREEN_REFRESH_PARTIAL = 0xFF,
} DS_SCREEN_REFRESH_MODE_E;


void ds_screen_refresh(DS_SCREEN_REFRESH_MODE_E mode);
void deep_sleep();

#endif

