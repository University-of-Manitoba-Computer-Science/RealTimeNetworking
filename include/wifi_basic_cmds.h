#ifndef WIFI_BASIC_CMDS_H
#define WIFI_BASIC_CMDS_H

#include <stdint.h>
#include <string.h>

void set_light_handler(uint8_t *response, uint8_t argc, char **argv);
void get_light_handler(uint8_t *response, uint8_t argc, char **argv);

#endif // WIFI_BASIC_CMDS_H
