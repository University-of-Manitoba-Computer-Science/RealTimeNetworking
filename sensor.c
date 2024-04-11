#include "sam.h"

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
#define COMMAND_CHECK_MS 1000UL


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

    

        PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
        updateOutput(0x11);
    

}

void off(){

    

        PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;
        updateOutput(0x00);
    

}

void on(){

    

        PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA14;
        updateOutput(0x7F);
    

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


//decodes our message
void decode_msg(uint8_t *msg){
    uint8_t tmp[2];
    if(msg[0] == (uint8_t)('g')){
        //gyro command block
        if(msg[1] == (uint8_t)('x')){
            //tx gyro_xyz_buff[0]
            tmp[0] = (uint8_t)((0XFF00&gyro_xyz_buff[0])>>8);
            tmp[1] = (uint8_t)(0X00FF&gyro_xyz_buff[0]);
            txUARTArr(SERCOM0_REGS, tmp, 2);
        }
        else if(msg[1] == (uint8_t)('y')){
            //tx gyro_xyz_buff[1]
            tmp[0] = (uint8_t)((0XFF00&gyro_xyz_buff[1])>>8);
            tmp[1] = (uint8_t)(0X00FF&gyro_xyz_buff[1]);
            txUARTArr(SERCOM0_REGS, tmp, 2);
        }
        else if(msg[1] == (uint8_t)('z')){
            //tx gyro_xyz_buff[2]
            tmp[0] = (uint8_t)((0XFF00&gyro_xyz_buff[2])>>8);
            tmp[1] = (uint8_t)(0X00FF&gyro_xyz_buff[2]);
            txUARTArr(SERCOM0_REGS, tmp, 2);
        }

    }
    else if(msg[0] == (uint8_t)('x')){
        //Accelerometer command block
        txMode(SERCOM0_REGS);
        if(msg[1] == (uint8_t)('x')){
            //tx xl_xyz_buff[0]
            tmp[0] = (uint8_t)((0XFF00&xl_xyz_buff[0])>>8);
            tmp[1] = (uint8_t)(0X00FF&xl_xyz_buff[0]);
            txUARTArr(SERCOM0_REGS, tmp, 2);
        }
        else if(msg[1] == (uint8_t)('y')){
            //tx xl_xyz_buff[1]
            tmp[0] = (uint8_t)((0XFF00&xl_xyz_buff[1])>>8);
            tmp[1] = (uint8_t)(0X00FF&xl_xyz_buff[1]);
            txUARTArr(SERCOM0_REGS, tmp, 2);
        }
        else if(msg[1] == (uint8_t)('z')){
            //tx xl_xyz_buff[2]
            tmp[0] = (uint8_t)((0XFF00&xl_xyz_buff[2])>>8);
            tmp[1] = (uint8_t)(0X00FF&xl_xyz_buff[2]);
            txUARTArr(SERCOM0_REGS, tmp, 2);
        }
        rxMode(SERCOM0_REGS);
    }
    else if(msg[0] == (uint8_t)('f')){
        //Fan command block
        updateOutput(msg[1]);

    }
    else if(msg[0] == (uint8_t)('l')){
        //Led command block

        if(msg[1] == (uint8_t)('0')){
            led = &off;
        }
        else if(msg[1] == (uint8_t)('1')){
            led = &on;
        }
        else if(msg[1] == (uint8_t)('2')){
            led = &flash;
        }

    }



}

static uint8_t extra         = 0;
static int     extra_is_used = 0;

//get our uart message and decode them to figure out what to do
void commandHandler(){
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
                            decode_msg(rx_data);
                            #ifndef NDEBUG
                                dbg_write_u8(rx_data,2);
                            #endif

                        }
                    } else {
                        uart_len = rxUART(SERCOM0_REGS, rx_data, 2);

                        if (uart_len == 2) {
                            decode_msg(rx_data);
                            #ifndef NDEBUG
                                dbg_write_u8(rx_data,2);
                            #endif
                        } else {
                            // handle byte with a missing second byte
                            extra         = rx_data[0];
                            extra_is_used = 1;
                        }
                    }
                }
 
            
}

void initAllPorts()
{
    // LED output
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
    //PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;

    port15Init();  // init button ports
    sercom2Init(); // init sercom2 -> i2c ports
    portUART(SERCOM0_REGS);    // init UART ports

} // initAllPorts

void initAllClks()
{
    clkButton();
    clkI2C();
    clkUART(SERCOM0_REGS);

} // initAllClks

void initAll()
{
    heartInit();
    initAllPorts();
    initAllClks();
    initI2C();
    initButton();
    initUART(SERCOM0_REGS);
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
    rxMode(SERCOM0_REGS);


    while (1) {
        __WFI();
        if ((get_ticks() % LED_FLASH_MS) == 0) {
            led();
            commandHandler();
        }
        sample();





/*         #ifndef NDEBUG
            if((get_ticks() % LED_FLASH_MS) == 0){
                dbg_write_str("Gyro x y z:");
                dbg_write_u16(gyro_xyz_buff,3);
                dbg_write_str(" \n");\
                
                dbg_write_str("XL x y z:");
                dbg_write_u16(xl_xyz_buff,3);
                dbg_write_str(" \n");

                uint16_t rpm = getRpm();
                dbg_write_str("Fan rpm:");
                dbg_write_u16(&rpm,1);
                dbg_write_str(" \n");
            }
        #endif   */

        
    }
    return 0;
}
