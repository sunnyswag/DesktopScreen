

#ifndef _DS_UI_FANS_H_
#define _DS_UI_FANS_H_

#include <stdint.h>

void ds_ui_fans_init();
void ds_ui_fans_show_init(uint8_t type);
void ds_ui_fans_set_fans_num(int fans);
void ds_ui_fans_set_aqifans_num(int fans);

#endif
