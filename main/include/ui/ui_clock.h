#ifndef _DS_UI_CLOCK_H_
#define _DS_UI_CLOCK_H_

#include <stdint.h>

void ds_ui_clock_add_time(unsigned int x, unsigned int y, uint8_t hour, uint8_t minute);
void ds_ui_clock_update_time(unsigned int x, unsigned int y, uint8_t hour, uint8_t minute);

#endif
