#include <sam.h>

#include "button.h"
#include "dcc_stdio.h"
#include "heart.h"
#include "i2c.h"
#include "uart.h"
#include "fan.h"

#define DEBUG_WAIT 10000000UL
// setup our heartbeat to be 1ms: we overflow at 1ms intervals with a 120MHz
// clock uses the SysTicks unit so that we get reliable debugging (timer stops
// on breakpoints)
#define MS_TICKS 2000UL
// #define MS_TICKS 120000UL
//  number of millisecond between LED flashes
#define LED_FLASH_MS  1000UL
#define GYRO_CHECK_MS 200UL


// NOTE: this overflows every ~50 days, so I'm not going to care here...
// volatile uint32_t msCount = 0;
volatile uint32_t secCount = 0;
static unsigned char i2c_rx_buff[READ_BUF_SIZE];

uint16_t xl_xyz_buff[3];
uint16_t gyro_xyz_buff[3];

void flash();
void on();
void off();

void (*led)() = &flash;

void flash(){

    if ((get_ticks() % LED_FLASH_MS) == 0) {

        PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
        updateOutput(0x7F);
    }

}

void off(){

    if ((get_ticks() % LED_FLASH_MS) == 0) {

        PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;
        updateOutput(0x00);
    }

}

void on(){

    if ((get_ticks() % LED_FLASH_MS) == 0) {

        PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA14;
        updateOutput(0xFF);
    }

}


void sampleG();
void sampleX();
void (*sample)() = &sampleG;

void sampleG(){

    if ((get_ticks() % GYRO_CHECK_MS) == 0) {

        sampleGyro(gyro_xyz_buff);
        
    }

    sample = &sampleX;

}

void sampleX(){

        if ((get_ticks() % GYRO_CHECK_MS) == 0) {

        sampleXL(xl_xyz_buff);
        
    }
    sample = &sampleG;
}

void initAllPorts()
{
    // LED output
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
    //PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;

    port15Init();  // init button ports
    sercom2Init(); // init sercom2 -> i2c ports
    portUART(SERCOM4_REGS);    // init UART ports

} // initAllPorts

void initAllClks()
{
    clkButton();
    clkI2C();
    clkUART(SERCOM4_REGS);

} // initAllClks

void initAll()
{
    heartInit();
    initAllClks();
    initAllPorts();
    initI2C();
    initButton();
    initUART(SERCOM4_REGS);
    fanInit();
    rpmInit();

    // init gyro stuff
    // accelOnlyMode();
}



// ISR for  external interrupt 15, add processing code as required...
void EIC_EXTINT_15_Handler()
{
    PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
    // clear the interrupt! and go to the next operating mode
    EIC_REGS->EIC_INTFLAG |= EXTINT15_MASK;
}

void test()
{
    dbg_write_str("beforee");
    PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
    dbg_write_str("afterr");
}

int main(void)
{
#ifndef NDEBUG
    for (int i = 0; i < DEBUG_WAIT; i++);
#endif

    // NOTE: the silkscreen on the curiosity board is WRONG! it's PB4 and PB5
    // NOT PA4 and PA5

    // see the header files within include/component for register definitions,
    // which align with the data sheet for the processor e.g. port.h contains
    // the masks and definitions for manipulating gpio

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

    initAll();

    // we want interrupts!
    __enable_irq();

    // some example logging calls
#ifndef NDEBUG
    dbg_write_str("~~~DEBUG ENABLED~~~\n");
#endif

    PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
    accelOnMode();
    gyroOnMode();
    // sleep until we have an interrupt
    while (1) {
        __WFI();
        led();
        sample();
        #ifndef NDEBUG
            if((get_ticks() % LED_FLASH_MS) == 0){
                dbg_write_str("Gyro x y z:");
                dbg_write_u16(gyro_xyz_buff,3);
                dbg_write_str(" \n");\
                
                dbg_write_str("XL x y z:");
                dbg_write_u16(xl_xyz_buff,3);
                dbg_write_str(" \n");
            }
        #endif  

        
    }
    return 0;
}
