#include "sam.h"
#include <stddef.h>
#include <stdbool.h>

//Look at the equation source is 16mhz target 1mhz
// #define UART_BUAD 0x8000
#define UART_BUAD 0UL

#define SERCOM0_TX_MODE 0x0
#define SERCOM0_RX_MODE 0x1
#define SERCOM5_TX_MODE 0x0
#define SERCOM5_RX_MODE 0x0

void clkUART();
void initUART();
void portUART();
void rs485Ports();

void txUART(sercom_registers_t*, uint8_t data);
uint8_t rxUART(sercom_registers_t*);