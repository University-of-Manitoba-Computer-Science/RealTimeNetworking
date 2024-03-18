#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stddef.h>

void init_spi();
uint8_t spi_write(uint8_t *data, uint8_t len);
uint8_t spi_read(uint8_t *data, uint8_t len);
void spi_select_device(uint32_t pin);
void spi_deselect_device(uint32_t pin);
uint8_t spi_io(void *tx_data, size_t tx_size, void *rx_data, size_t rx_size);

#endif