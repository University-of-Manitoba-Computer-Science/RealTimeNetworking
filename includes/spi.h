#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void init_spi();
uint8_t spi_write(uint8_t *data, uint32_t len);
uint8_t spi_read(uint8_t *data, uint32_t len);
void spi_select_device(uint32_t pin);
void spi_deselect_device(uint32_t pin);

#endif