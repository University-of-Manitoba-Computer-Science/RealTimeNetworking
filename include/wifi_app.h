#ifndef WIFI_APP_H
#define WIFI_APP_H

#include <wifi8.h>
#include <string.h>

#define MAIN_TCP_SERVER_PORT 8080

typedef struct s_msg_wifi_product
{
    uint8_t name[30];

} t_msg_wifi_product;

void init_wifi_app();
void init_wifi_socket();
void init_access_point();
void print_wifi_version();
void tick_wifi_app();

#endif // WIFI_APP_H