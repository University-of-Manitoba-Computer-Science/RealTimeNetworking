#include <assert.h>
#include <sam.h>

#include "button.h"
#include "can.h"
#include "dcc_stdio.h"
#include "heart.h"

// number of millisecond between LED flashes
#define LED_FLASH_MS 1000UL

// NOTE: this overflows every ~50 days, so I'm not going to care here...
volatile uint32_t msCount = 0;

// Fires every 1ms
void SysTick_Handler()
{
    msCount++;
}

// ISR for  external interrupt 15, add processing code as required...
void EIC_EXTINT_15_Handler()
{
    // clear the interrupt! and go to the next operating mode
    EIC_REGS->EIC_INTFLAG           |= EXTINT15_MASK;
    PORT_REGS->GROUP[0].PORT_OUTTGL  = PORT_PA14;
}

// pull down tx and rx in interrupt

int main(void)
{
#ifndef NDEBUG
    for (int i = 0; i < 100000; i++);
#endif

    // LED output
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;

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
    buttonInit();
    canInit();

    // we want interrupts!
    __enable_irq();

    uint8_t test = 0; // TODO remove this once we have actual data to send

    // sleep until we have an interrupt
    while (1) {
        __WFI();
        if ((msCount % LED_FLASH_MS) == 0) {
            uint8_t data[1] = {test++};
            queue_message(data, 1);

            PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;

            uint8_t rx_data[16];
            while ((dequeue_message(rx_data, 16)) != -1);
        }
    }
    return 0;
}
