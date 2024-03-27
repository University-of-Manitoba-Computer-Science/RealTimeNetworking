#include "sam.h"
#include <stddef.h>
#include <stdbool.h>

#define UART_BUAD 0x0FFF
//#define TX_MODE 0x0
#define TX_MODE 0x2 //this mode uses pad0 as the tx sercom pad
#define RX_MODE 0x1

void clkUART();
void initUART();
void portUART();

void txUART(uint8_t data);
uint8_t rxUART();