#include "wifi_basic_cmds.h"

#include "dcc_stdio.h"

uint8_t light_status = 0;

void set_light_handler(uint8_t *response, uint8_t argc, char **argv)
{
    if (argc == 2)
    {
        if (strncmp(argv[1], "on", 2) == 0)
        {
            light_status = 1;
            strcat(response, "Light is on\n");
            return;
        }
        else if (strncmp(argv[1], "off", 3) == 0)
        {
            light_status = 0;
            strcat(response, "Light is off\n");
            return;
        }
    }
    strcat(response, "Invalid arguments\n");
}

void get_light_handler(uint8_t *response, uint8_t argc, char **argv)
{
    if (light_status == 1)
    {
        strcat(response, "Light is on\n");
    }
    else
    {
        strcat(response, "Light is off\n");
    }
}