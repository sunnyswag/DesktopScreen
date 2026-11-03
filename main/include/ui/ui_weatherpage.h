

#ifndef _DS_UI_WEATHERPAGE_H_
#define _DS_UI_WEATHERPAGE_H_

#include <stdint.h>

typedef enum
{
    VALUE_CITY = 0,
    VALUE_WEATHER,
    VALUE_CODE,
    VALUE_TEMP,
} VALUE_TYPE_T;

void ds_ui_weather_set(VALUE_TYPE_T type, char *value);
void ds_ui_weather_init();
void ds_ui_weather_show(uint8_t type);

#endif
