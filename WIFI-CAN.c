#include "sam.h"
#include "dcc_stdio.h"
#include "heart.h"
#include "spi.h"
#include "wifi8.h"
#include "wifi_app.h"
#include "wifi_cmd.h"
#include "wifi_basic_cmds.h"
#include <string.h>
#include <assert.h>

int main(void)
{
    // setup led output on PA14
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;

    // enable cache (see main branch for tradeoffs)
    if ((CMCC_REGS->CMCC_SR & CMCC_SR_CSTS_Msk) == 0)
        CMCC_REGS->CMCC_CTRL = CMCC_CTRL_CEN_Msk;

    // sleep to idle (wake on interrupts)
    PM_REGS->PM_SLEEPCFG |= PM_SLEEPCFG_SLEEPMODE_IDLE;

    heartInit();

    // we want interrupts!
    __enable_irq();

    // initialize the SPI
    init_spi();

    init_wifi_app();
    print_wifi_version();
    init_access_point();
    init_wifi_socket();

    register_wifi_cmd("help", set_light_handler);
    register_wifi_cmd("set_light", set_light_handler);
    register_wifi_cmd("get_light", get_light_handler);

    // led indicates when server is running and ready to accept connections
    PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA14;

    while (1)
    {
        tick_wifi_app();
    }
    return 0;
}
