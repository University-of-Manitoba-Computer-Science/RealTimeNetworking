#include "sam.h"
#include <stddef.h>
#include <stdbool.h>

#define UART_BUAD 0x22

void clkUART();
void initUART();
void portUART();

void txUART(uint8_t data);
uint8_t rxUART();