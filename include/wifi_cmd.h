#ifndef WIFI_COMMANDS_H
#define WIFI_COMMANDS_H

#include <stdint.h>
#include <string.h>

int32_t get_cmd_index(const char *cmd);
uint8_t register_wifi_cmd(const char *cmd, void (*func)(uint8_t *response, uint8_t argc, char **argv));
void run_wifi_cmd(int32_t cmd, uint8_t *response, uint8_t argc, char **argv);

void help_handler(uint8_t *response, uint8_t argc, char **argv);

#endif // WIFI_COMMANDS_H