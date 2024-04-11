#include <sam.h>

#include "can.h"
#include "dcc_stdio.h"
#include "heart.h"
#include "uart.h"

#define CAN_ID    0x200
#define CAN_RX_ID 0x201

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

    // setup heart
    heartInit();

    // setup CAN
    uint16_t rcv_id = CAN_RX_ID;
    canInit(&rcv_id, 1);

    // setup RS-485
    portUART(SERCOM0_REGS);
    clkUART(SERCOM0_REGS);
    initUART(SERCOM0_REGS);
    rxMode(SERCOM0_REGS); // enable SERCOM0 reading

    // we want interrupts!
    __enable_irq();

    uint8_t extra         = 0;
    int     extra_is_used = 0;

    // sleep until we have an interrupt
    while (1) {
        __WFI();

        // forward messages once every second
        if ((get_ticks() % 1000) == 0) {
            int can_len = 0;
            while (can_len != -1) {
                // receive CAN message
                uint8_t rx_data[2];
                can_len = dequeue_message(rx_data, 2);

                // send the message as a pair of UART messages if successful
                if (can_len > 0) {
                    txMode(SERCOM0_REGS);
                    txUARTArr(SERCOM0_REGS, rx_data, 2);
                    rxMode(SERCOM0_REGS);
                }
            }

            int uart_len = 1;
            while (uart_len != 0) {
                uint8_t rx_data[2];

                if (extra_is_used) {
                    // handle second byte of pair if first was unused
                    rx_data[0] = extra;
                    uart_len   = rxUART(SERCOM0_REGS, &extra, 1);
                    if (uart_len > 0) {
                        rx_data[1]    = extra;
                        extra_is_used = 0;
                        queue_message(CAN_ID, rx_data, 2);
                    }
                } else {
                    uart_len = rxUART(SERCOM0_REGS, rx_data, 2);

                    if (uart_len == 2) {
                        queue_message(CAN_ID, rx_data, 2);
                    } else {
                        // handle byte with a missing second byte
                        extra         = rx_data[0];
                        extra_is_used = 1;
                    }
                }
            }
        }
    }
    return 0;
}
