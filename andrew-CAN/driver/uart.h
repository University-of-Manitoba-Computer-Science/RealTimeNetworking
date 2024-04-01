#include "sam.h"
#include <stddef.h>
#include <stdbool.h>

//Look at the equation source is 16mhz target 1mhz
// #define UART_BUAD 0x8000
#define UART_BUAD 0UL

#define TX_MODE 0x0
#define RX_MODE 0x1

void clkUART();
void initUART();
void portUART();

void txUART(sercom_registers_t*, uint8_t data);
uint8_t rxUART(sercom_registers_t*);