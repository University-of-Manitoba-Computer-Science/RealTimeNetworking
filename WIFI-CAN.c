#include "sam.h"
#include "dcc_stdio.h"
#include "heart.h"
#include "spi.h"
#include "wifi8.h"
#include "wifi_app.h"
#include "wifi_cmd.h"
#include "wifi_basic_cmds.h"
#include "can.h"
#include <string.h>
#include <assert.h>

#define CAN_ID 0x201
#define CAN_RX_ID 0x200

void send_can_handler(uint8_t *response, uint8_t argc, char **argv);

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

    // initialize the CAN peripheral
    uint16_t rcv_id = CAN_RX_ID;
    canInit(&rcv_id, 1);

    // we want interrupts!
    __enable_irq();

    // initialize the SPI
    init_spi();

    init_wifi_app();
    print_wifi_version();
    init_access_point();
    init_wifi_socket();

    register_wifi_cmd("help", help_handler);
    register_wifi_cmd("set_light", set_light_handler);
    register_wifi_cmd("get_light", get_light_handler);
    register_wifi_cmd("send_can", send_can_handler);

    // led indicates when server is running and ready to accept connections
    PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA14;

    while (1)
    {
        __WFI();

        tick_wifi_app();
    }
    return 0;
}

void send_can_handler(uint8_t *response, uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        queue_message(CAN_ID, argv[1], 8);
        strcat(response, "CAN message has been sent!\n");
        return;
    }
    else
    {
        strcat(response, "Invalid arguments\n");
    }
}