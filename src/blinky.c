#include "sam.h"
#include "dcc_stdio.h"
#include "heartbeat.h"
#include "spi.h"
#include "wifi8.h"
#include <string.h>
#include <assert.h>

#define MAIN_TCP_SERVER_PORT 8080
#define MAX_RESPONSE_SIZE 4096
#define MAX_RESPONSE_ITEM_SIZE 128
#define MAX_RESPONSE_ITEMS 32

static uint8_t gau8_socket_test_buffer[1024] = {0};

static int8_t tcp_server_socket = -1;
static int8_t tcp_client_socket = -1;
wifi8_sockaddr_in_t addr;

static uint8_t ap_status;

static void ap_wifi_cb(uint8_t u8_msg_type, void *pv_msg);
static void socket_cb(int8_t sock, uint8_t u8_msg, void *pv_msg);

static wifi8_t wifi8;

uint8_t get_http_response(char *response);
void add_to_response(char *str, uint32_t len);
void clear_response();

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

  // set the default configuration

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

  wifi8_socket_init(&wifi8);
  addr.sin_family = 2;
  addr.sin_port = (uint16_t)((((uint16_t)((MAIN_TCP_SERVER_PORT))) << 8) | (((uint16_t)((MAIN_TCP_SERVER_PORT))) >> 8));
  addr.sin_addr.s_addr = 0;

  // led indicates when server is running and ready to accept connections
  PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA14;

  while (1)
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
  return 0;
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
  if (tcp_server_socket < 0)
  {
    return;
  }

  dbg_write_str("socket_cb: ");
  dbg_write_u8(&u8_msg, 1);
  dbg_write_str("\n");

  switch (u8_msg)
  {
  case SOCKET_MSG_BIND:
  {
    wifi8_socket_bind_msg_t *pstr_bind = (wifi8_socket_bind_msg_t *)pv_msg;
    if (pstr_bind && pstr_bind->status == 0)
    {
      delay_ms(500);
      wifi8_socket_listen(&wifi8, sock, 0);
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

      char http_response[MAX_RESPONSE_SIZE];
      uint32_t response_size = get_http_response(http_response);
      wifi8_socket_send(&wifi8, tcp_client_socket, http_response, response_size);

      uint8_t result = wifi8_socket_receive(&wifi8, pstr_accept->sock, gau8_socket_test_buffer, sizeof(gau8_socket_test_buffer), 5000);
      if (result != WIFI8_OK)
      {
        dbg_write_str("socket_cb: receive error!\n");
        wifi8_socket_close(&wifi8, tcp_client_socket);
        tcp_client_socket = -1;
      }
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
    wifi8_socket_receive(&wifi8, sock, gau8_socket_test_buffer, sizeof(gau8_socket_test_buffer), 5000);
  }
  break;
  case SOCKET_MSG_RECV:
  {
    wifi8_socket_recv_msg_t *pstr_recv = (wifi8_socket_recv_msg_t *)pv_msg;
    if (pstr_recv && pstr_recv->s16_buffer_size > 0)
    {
      dbg_write_str("socket_cb: recv success!\n");
      if ((strchr(pstr_recv->pu8_buffer, 13) != 0) || (strchr(pstr_recv->pu8_buffer, 10) != 0))
      {
        char http_response[MAX_RESPONSE_SIZE];
        uint32_t response_size = get_http_response(http_response);
        wifi8_socket_send(&wifi8, sock, http_response, response_size);
      }
      else
      {
        wifi8_socket_receive(&wifi8, sock, gau8_socket_test_buffer, sizeof(gau8_socket_test_buffer), 5000);
      }
      memset(pstr_recv->pu8_buffer, 0, pstr_recv->s16_buffer_size);
    }
    else
    {
      dbg_write_str("socket_cb: recv error!\n");
      wifi8_socket_close(&wifi8, tcp_client_socket);
      tcp_client_socket = -1;
    }
  }
  break;
  default:
  {
    break;
  }
  }
}

static char response_items[MAX_RESPONSE_ITEMS][MAX_RESPONSE_ITEM_SIZE];
static uint8_t response_item_count = 0;
static uint8_t start_item_idx = 0;
void add_to_response(char *str, uint32_t len)
{
  if (response_item_count >= MAX_RESPONSE_ITEMS)
  {
    strncpy(response_items[start_item_idx], str, MAX_RESPONSE_ITEM_SIZE - 1);
    start_item_idx = (start_item_idx + 1) % MAX_RESPONSE_ITEMS;
  }
  else
  {
    strncpy(response_items[response_item_count], str, MAX_RESPONSE_ITEM_SIZE - 1);
    response_item_count++;
  }
}

uint8_t get_http_response(char *response)
{
  char *http_header = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n[";

  strncpy(response, http_header, MAX_RESPONSE_ITEM_SIZE - 1);

  for (uint8_t i = 0; i < response_item_count; i++)
  {
    strncat(response, "\"", MAX_RESPONSE_ITEM_SIZE - 1);
    strncat(response, response_items[(start_item_idx + i) % MAX_RESPONSE_ITEMS], MAX_RESPONSE_ITEM_SIZE - 1);
    strncat(response, "\"", MAX_RESPONSE_ITEM_SIZE - 1);
    if (i < response_item_count - 1)
    {
      strncat(response, ",", MAX_RESPONSE_ITEM_SIZE - 1);
    }
    else
    {
      strncat(response, "]", MAX_RESPONSE_ITEM_SIZE - 1);
    }
  }

  return strlen(response);
}

void clear_response()
{
  response_item_count = 0;
  start_item_idx = 0;
  memset(response_items, 0, sizeof(response_items));
}