#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void init_spi();
uint8_t spi_write(uint8_t *data, uint8_t len);
uint8_t spi_read(uint8_t *data, uint8_t len);

#endif