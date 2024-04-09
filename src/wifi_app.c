#include "wifi_app.h"

static t_msg_wifi_product msg_wifi_product = {.name = "WiFi 8 Click"};

static uint8_t gau8_socket_test_buffer[1024] = {0};

static int8_t tcp_server_socket = -1;
static int8_t tcp_client_socket = -1;
wifi8_sockaddr_in_t addr;

static uint8_t ap_status;

static wifi8_t wifi8;

static void ap_wifi_cb(uint8_t u8_msg_type, void *pv_msg);
static void socket_cb(int8_t sock, uint8_t u8_msg, void *pv_msg);

void print_wifi_version()
{
    wifi8_m2m_rev_t fw_version;
    if (WIFI8_OK == wifi8_get_full_firmware_version(&wifi8, &fw_version))
    {
        dbg_write_str("Firmware Build ");
        dbg_write_str(fw_version.build_date);
        dbg_write_str(" Time ");
        dbg_write_str(fw_version.build_time);
        dbg_write_str("\n");
    }
    else
    {
        dbg_write_str("Error getting firmware version\n");
        for (;;)
            ;
    }
}

void tick_wifi_app()
{
    wifi8_handle_events(&wifi8);

    if (tcp_server_socket < 0)
    {

        if ((tcp_server_socket = wifi8_socket_create(&wifi8, 2, 1, 0)) < 0)
        {
            dbg_write_str("error creating socket\n");
        }
        else
        {
            wifi8_socket_bind(&wifi8, tcp_server_socket, (wifi8_sockaddr_t *)&addr,
                              sizeof(wifi8_sockaddr_in_t));
        }
    }
}

void init_wifi_socket()
{
    wifi8_socket_init(&wifi8);
    addr.sin_family = 2;
    addr.sin_port = (uint16_t)((((uint16_t)((MAIN_TCP_SERVER_PORT))) << 8) | (((uint16_t)((MAIN_TCP_SERVER_PORT))) >> 8));
    addr.sin_addr.s_addr = 0;
}

void init_access_point()
{
    wifi8_m2m_ap_config_t ap_config;

    memset(&ap_config, 0, sizeof(wifi8_m2m_ap_config_t));
    strcpy((char *)ap_config.au8ssid, "CANWeDoIt?");
    ap_config.u8_listen_channel = 1;
    ap_config.u8_sec_type = M2M_WIFI_SEC_OPEN;
    ap_config.u8_ssid_hide = 0;
    ap_config.au8dhcp_server_ip[0] = 192;
    ap_config.au8dhcp_server_ip[1] = 168;
    ap_config.au8dhcp_server_ip[2] = 1;
    ap_config.au8dhcp_server_ip[3] = 1;

    if (ap_status == M2M_WIFI_DISCONNECTED)
    {
        if (WIFI8_OK != wifi8_start_ap(&wifi8, &ap_config))
        {
            dbg_write_str("error starting access point\n");
            for (;;)
                ;
        }
        else
        {
            dbg_write_str("access point started\n");
        }
    }
}

void init_wifi_app()
{
    // wait for wifi
    delay_ms(2000);

    // initialize the wifi
    err_t result = wifi8_init(&wifi8);
    dbg_write_str("wifi8_init:");
    dbg_write_u8(&result, 1);

    delay_ms(2000);

    if (WIFI8_OK != wifi8_default_cfg(&wifi8))
    {
        dbg_write_str("error setting default config\n");
        for (;;)
            ;
    }
    dbg_write_str("wifi chip initialization completed.\n");

    wifi8.app_wifi_cb = ap_wifi_cb;
    wifi8.app_socket_cb = socket_cb;
    ap_status = M2M_WIFI_DISCONNECTED;
}

static void ap_wifi_cb(uint8_t u8_msg_type, void *pv_msg)
{
    switch (u8_msg_type)
    {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        wifi8_m2m_wifi_state_changed_t *pstr_wifi_state = (wifi8_m2m_wifi_state_changed_t *)pv_msg;

        if (pstr_wifi_state->u8_curr_state == M2M_WIFI_CONNECTED)
        {
            dbg_write_str("wifi_cb: client connected\n");
        }
        else if (pstr_wifi_state->u8_curr_state == M2M_WIFI_DISCONNECTED)
        {
            dbg_write_str("wifi_cb: client disconnected\n");
        }

        break;
    }
    case M2M_WIFI_REQ_DHCP_CONF:
    {
        dbg_write_str("wifi_cb: receive ip address: ");
        break;
    }
    default:
    {
        break;
    }
    }
}

static void socket_cb(int8_t sock, uint8_t u8_msg, void *pv_msg)
{
    switch (u8_msg)
    {
    case SOCKET_MSG_BIND:
    {
        wifi8_socket_bind_msg_t *pstr_bind = (wifi8_socket_bind_msg_t *)pv_msg;
        if (pstr_bind && pstr_bind->status == 0)
        {
            dbg_write_str("socket_cb: bind success!\n");
            delay_ms(500);
            wifi8_socket_listen(&wifi8, tcp_server_socket, 0);
        }
        else
        {
            dbg_write_str("socket_cb: bind error!\n");
            wifi8_socket_close(&wifi8, tcp_server_socket);
            tcp_server_socket = -1;
        }
    }
    break;
    case SOCKET_MSG_LISTEN:
    {
        wifi8_socket_listen_msg_t *pstr_listen = (wifi8_socket_listen_msg_t *)pv_msg;
        if (pstr_listen && pstr_listen->status == 0)
        {
            dbg_write_str("socket_cb: listen success!\n");
        }
        else
        {
            dbg_write_str("socket_cb: listen error!\n");
            wifi8_socket_close(&wifi8, tcp_server_socket);
            tcp_server_socket = -1;
        }
    }
    break;
    case SOCKET_MSG_ACCEPT:
    {
        wifi8_socket_accept_msg_t *pstr_accept = (wifi8_socket_accept_msg_t *)pv_msg;
        if (pstr_accept)
        {
            dbg_write_str("socket_cb: accept success!\n");
            tcp_client_socket = pstr_accept->sock;
            wifi8_socket_receive(&wifi8, tcp_client_socket, gau8_socket_test_buffer, sizeof(gau8_socket_test_buffer), 0);
        }
        else
        {
            dbg_write_str("socket_cb: accept error!\n");
            wifi8_socket_close(&wifi8, tcp_server_socket);
            tcp_server_socket = -1;
        }
    }
    break;
    case SOCKET_MSG_SEND:
    {
        dbg_write_str("socket_cb: send success!\n");
        wifi8_socket_close(&wifi8, tcp_client_socket);
        tcp_client_socket = -1;
    }
    break;
    case SOCKET_MSG_RECV:
    {
        wifi8_socket_recv_msg_t *pstr_recv = (wifi8_socket_recv_msg_t *)pv_msg;
        if (pstr_recv && pstr_recv->s16_buffer_size > 0)
        {
            if ((strchr(pstr_recv->pu8_buffer, 13) != 0) || (strchr(pstr_recv->pu8_buffer, 10) != 0))
            {
                wifi8_socket_send(&wifi8, tcp_client_socket, &msg_wifi_product, sizeof(t_msg_wifi_product));
            }
            else
            {
                wifi8_socket_receive(&wifi8, tcp_client_socket, gau8_socket_test_buffer, sizeof(gau8_socket_test_buffer), 0);
            }
            memset(pstr_recv->pu8_buffer, 0, pstr_recv->s16_buffer_size);
        }
        else
        {
            dbg_write_str("socket_cb: close socket!\n");
            wifi8_socket_close(&wifi8, tcp_server_socket);
            tcp_server_socket = -1;
        }
    }
    break;
    default:
    {
        break;
    }
    }
}
