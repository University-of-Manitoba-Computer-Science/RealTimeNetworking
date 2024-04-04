#include "sam.h"
#include <stddef.h>
#include <stdbool.h>

//Look at the equation source is 16mhz target 1mhz
// #define UART_BUAD 0x8000
#define UART_BUAD 0UL

#define SERCOM0_TX_MODE 0x0
#define SERCOM0_RX_MODE 0x1
#define SERCOM4_TX_MODE 0x0
#define SERCOM4_RX_MODE 0x1
#define TX_DELAY_MS 4000UL

void clkUART();
void initUART();
void portUART();

void rxMode(sercom_registers_t*);
void txMode(sercom_registers_t*);

void txUART(sercom_registers_t*, uint8_t data);
void txUARTArr(sercom_registers_t* sercom, uint8_t * data, uint8_t len);
uint8_t rxUART(sercom_registers_t*);
