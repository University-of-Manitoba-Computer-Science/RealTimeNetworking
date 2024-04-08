#include <sam.h>

#include "can.h"
#include "heart.h"
#include "uart.h"

#define MAX_RX_DATA_LEN 16

#define CAN_ID 0x200

// NOTE: this overflows every ~50 days, so I'm not going to care here...
volatile uint32_t msCount = 0;

// Fires every 1ms
void SysTick_Handler()
{
    msCount++;
}

int main(void)
{
#ifndef NDEBUG
    for (int i = 0; i < 100000; i++);
#endif

    // enable cache
    // tradeoff: +: really helps with repeated code/data (like when doing
    // animations in a game)
    //           -: results in non-deterministic run-times
    //           +: there *is* a way to lock lines of cache to keep hard
    //           deadline code/data pinned in the cache
    if ((CMCC_REGS->CMCC_SR & CMCC_SR_CSTS_Msk) == 0)
        CMCC_REGS->CMCC_CTRL = CMCC_CTRL_CEN_Msk;

    // sleep to idle (wake on interrupts)
    PM_REGS->PM_SLEEPCFG |= PM_SLEEPCFG_SLEEPMODE_IDLE;

    heartInit();
    uint16_t tmp = CAN_ID + 1; // TODO choose ids to accept
    canInit(&tmp, 1);
    portUART();
    clkUART();
    initUART();
    rxMode(SERCOM0_REGS);

    // we want interrupts!
    __enable_irq();

    // sleep until we have an interrupt
    while (1) {
        __WFI();
        if ((msCount % 1000) == 0) {
            // check for waiting CAN data and send it over RS485
            int can_len = 0;
            while (can_len != -1) {
                uint8_t rx_data[MAX_RX_DATA_LEN];
                can_len = dequeue_message(rx_data, MAX_RX_DATA_LEN);

                if (can_len > 0) {
                    txUARTArr(SERCOM0_REGS, rx_data, can_len);
                }
            }

            while (SERCOM0_REGS->USART_INT.SERCOM_INTFLAG &
                   SERCOM_USART_INT_INTFLAG_TXC_Msk) {
                uint8_t data = rxUART(SERCOM0_REGS);

                queue_message(CAN_ID, &data, 1);
            }
        }
    }
    return 0;
}
