

#ifndef _DS_SPI_H_
#define _DS_SPI_H_

#include <stdint.h>

typedef enum
{
    SPI_BYTE_COMMAND = 0,
    SPI_BYTE_DATA = 1,
} SPI_BYTE_TYPE_E;
void spi_send_byte( uint8_t value, SPI_BYTE_TYPE_E type);
void screen_spi_init(void);

#endif

