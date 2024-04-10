#include "wifi_cmd.h"

#define MAX_WIFI_CMDS 10

typedef struct
{
    const char *cmd;
    void (*func)(uint8_t *response, uint8_t argc, char **argv);
} t_wifi_cmd;

t_wifi_cmd wifi_cmds[MAX_WIFI_CMDS];

int32_t get_cmd_index(const char *cmd)
{
    for (int32_t i = 0; i < MAX_WIFI_CMDS; i++)
    {
        if (wifi_cmds[i].cmd != 0 && strcmp(wifi_cmds[i].cmd, cmd) == 0)
        {
            return i;
        }
    }
    return -1;
}

uint8_t register_wifi_cmd(const char *cmd, void (*func)(uint8_t *response, uint8_t argc, char **argv))
{
    int32_t index = get_cmd_index(cmd);
    if (index == -1)
    {
        for (int32_t i = 0; i < MAX_WIFI_CMDS; i++)
        {
            if (wifi_cmds[i].cmd == 0)
            {
                wifi_cmds[i].cmd = cmd;
                wifi_cmds[i].func = func;
                return 1;
            }
        }
    }
    return 0;
}

void run_wifi_cmd(int32_t cmd, uint8_t *response, uint8_t argc, char **argv)
{
    if (cmd >= 0 && cmd < MAX_WIFI_CMDS)
    {
        wifi_cmds[cmd].func(response, argc, argv);
    }
}

void help_handler(uint8_t *response, uint8_t argc, char **argv)
{
    strcat(response, "Available commands:\n");
    for (int32_t i = 0; i < MAX_WIFI_CMDS; i++)
    {
        if (wifi_cmds[i].cmd != 0)
        {
            strcat(response, wifi_cmds[i].cmd);
            strcat(response, "\n");
        }
    }
}