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
void set_gyro_handler(uint8_t *response, uint8_t argc, char **argv);
void set_fan_handler(uint8_t *response, uint8_t argc, char **argv);
void set_blinky_handler(uint8_t *response, uint8_t argc, char **argv);

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

    register_wifi_cmd("set_gyro", set_gyro_handler);
    register_wifi_cmd("set_fan", set_fan_handler);
    register_wifi_cmd("set_blinky", set_blinky_handler);

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

void set_gyro_handler(uint8_t *response, uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        uint8_t command[2] = {'g'};

        if (strncmp(argv[1], "x", 1) == 0)
        {
            command[1] = 'x';
        }
        else if (strncmp(argv[1], "y", 1) == 0)
        {
            command[1] = 'y';
        }
        else if (strncmp(argv[1], "z", 1) == 0)
        {
            command[1] = 'z';
        }
        else
        {
            strcat(response, "Invalid argument (x/y/z)\n");
            return;
        }

        queue_message(CAN_ID, command, 2);
        strcat(response, "Gyro axis command sent!\n");
        return;
    }
    else
    {
        strcat(response, "Invalid arguments\n");
    }
}

void set_fan_handler(uint8_t *response, uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        uint8_t command[2] = {'f'};
        uint8_t speed = atoi(argv[1]);

        if (0 <= speed && speed <= 100)
        {
            command[1] = speed * 255 / 100;
        }
        else
        {
            strcat(response, "Invalid argument (0-100)\n");
            return;
        }

        queue_message(CAN_ID, command, 2);
        strcat(response, "Fan speed command sent!\n");
        return;
    }
    else
    {
        strcat(response, "Invalid arguments\n");
    }
}

void set_blinky_handler(uint8_t *response, uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        uint8_t command[2] = {'l'};

        if (strncmp(argv[1], "off", 3) == 0)
        {
            command[1] = '0';
        }
        else if (strncmp(argv[1], "on", 2) == 0)
        {
            command[1] = '1';
        }
        else if (strncmp(argv[1], "flashing", 8) == 0)
        {
            command[1] = '2';
        }
        else
        {
            strcat(response, "Invalid argument (on/off/flashing)\n");
            return;
        }

        queue_message(CAN_ID, command, 2);
        strcat(response, "Light mode command sent!\n");
        return;
    }
    else
    {
        strcat(response, "Invalid arguments\n");
    }
}
