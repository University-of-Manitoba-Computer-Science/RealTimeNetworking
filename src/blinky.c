#include "sam.h"
#include "dcc_stdio.h"
#include "heartbeat.h"
#include "spi.h"
#include "wifi8.h"
#include <string.h>
#include <assert.h>

#define EXTINT15_MASK 0x8000

void buttonInit()
{
  // switch input on PA15, processed as an external interrupt
  PORT_REGS->GROUP[0].PORT_DIRCLR = PORT_PA15;
  PORT_REGS->GROUP[0].PORT_PINCFG[15] |= PORT_PINCFG_PMUXEN_Msk;
  PORT_REGS->GROUP[0].PORT_PMUX[7] |= PORT_PMUX_PMUXO_A;

  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA15;
  PORT_REGS->GROUP[0].PORT_PINCFG[15] |= PORT_PINCFG_PULLEN_Msk;

  // have to enable the interrupt line in the system level REG
  NVIC_EnableIRQ(EIC_EXTINT_15_IRQn);

  MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_EIC_Msk;

  // the button is noisy so we keep it slow and lots of samples for filtering -- not great but it does the job (usually)
  EIC_REGS->EIC_CONFIG[1] |= ((uint32_t)(EXTINT15_MASK) << 16) | EIC_CONFIG_SENSE7_RISE;
  EIC_REGS->EIC_DEBOUNCEN |= EXTINT15_MASK;
  EIC_REGS->EIC_DPRESCALER |= EIC_DPRESCALER_TICKON_Msk | EIC_DPRESCALER_STATES1_Msk | EIC_DPRESCALER_PRESCALER1_DIV64;
  EIC_REGS->EIC_INTENSET |= EXTINT15_MASK;
  EIC_REGS->EIC_CTRLA |= EIC_CTRLA_CKSEL_CLK_ULP32K | EIC_CTRLA_ENABLE_Msk;
}

// button press handler
void EIC_EXTINT_15_Handler()
{
  EIC_REGS->EIC_INTFLAG |= EXTINT15_MASK;
}

uint8_t count = 0;

/** Wi-Fi Settings */
#define MAIN_WLAN_SSID "Breton"             /**< Destination SSID */
#define MAIN_WLAN_AUTH M2M_WIFI_SEC_WPA_PSK /**< Security type */
#define MAIN_WLAN_PSK "Pepper2332"          /**< Password for Destination SSID */
#define MAIN_TCP_SERVER_PORT 8080           /**< TCP Server port for client connection */

typedef struct s_msg_wifi_product
{
  uint8_t name[30];

} t_msg_wifi_product;

static t_msg_wifi_product msg_wifi_product =
    {
        .name = "WiFi 8 Click"};

static uint8_t gau8_socket_test_buffer[1024] = {0};

static int8_t tcp_server_socket = -1;
static int8_t tcp_client_socket = -1;
wifi8_sockaddr_in_t addr;

static uint8_t wifi_connected;

static uint8_t scan_request_index = 0;

static uint8_t num_found_ap = 0;

static void wifi_cb(uint8_t u8_msg_type, void *pv_msg);

static void socket_cb(int8_t sock, uint8_t u8_msg, void *pv_msg);

static wifi8_t wifi8;

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
  buttonInit();

  // we want interrupts!
  __enable_irq();

  // initialize the SPI
  init_spi();

  // wait for wifi
  delay_ms(2000);

  // initialize the wifi
  err_t result = wifi8_init(&wifi8);
  printf("wifi8_init: %d\n", result);

  delay_ms(2000);

  if (WIFI8_OK != wifi8_default_cfg(&wifi8))
  {
    printf("error setting default config\n");
    for (;;)
      ;
  }
  printf("wifi chip initialization completed.\n");

  wifi8.app_wifi_cb = wifi_cb;
  wifi8.app_socket_cb = socket_cb;
  wifi_connected = M2M_WIFI_DISCONNECTED;

  // set the default configuration

  wifi8_m2m_rev_t fw_version;
  if (WIFI8_OK == wifi8_get_full_firmware_version(&wifi8, &fw_version))
  {
    printf("Firmware HIF (%u) : %u.%u \n",
           ((uint16_t)(((fw_version.u16_firmware_hif_info) >> (14)) & (0x3))),
           ((uint16_t)(((fw_version.u16_firmware_hif_info) >> (8)) & (0x3f))),
           ((uint16_t)(((fw_version.u16_firmware_hif_info) >> (0)) & (0xff))));
    printf("Firmware ver   : %u.%u.%u \n",
           (uint16_t)fw_version.u8_firmware_major,
           (uint16_t)fw_version.u8_firmware_minor,
           (uint16_t)fw_version.u8_firmware_patch);
    printf("Firmware Build %s Time %s\n", fw_version.build_date, fw_version.build_time);
  }
  else
  {
    printf("error reading full firmware version.\n");
    for (;;)
      ;
  }

  if (wifi_connected == M2M_WIFI_DISCONNECTED)
  {
    if (WIFI8_OK != wifi8_connect(&wifi8, MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID),
                                  MAIN_WLAN_AUTH, MAIN_WLAN_PSK, M2M_WIFI_CH_ALL))
    {
      printf("Connection error\n");
      for (;;)
        ;
    }
    else
    {
      printf("Connecting...\n");
    }
  }

  while (wifi_connected != M2M_WIFI_CONNECTED)
  {
    wifi8_handle_events(&wifi8);
  }

  printf("Connected\n");

  wifi8_socket_init(&wifi8);
  addr.sin_family = 2;
  addr.sin_port = (uint16_t)((((uint16_t)((MAIN_TCP_SERVER_PORT))) << 8) | (((uint16_t)((MAIN_TCP_SERVER_PORT))) >> 8));
  addr.sin_addr.s_addr = 0;

  // led indicates when server is running and ready to accept connections
  PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA14;
  printf("main: starting server\n");

  while (1)
  {

    wifi8_handle_events(&wifi8);

    if (tcp_server_socket < 0)
    {

      if ((tcp_server_socket = wifi8_socket_create(&wifi8, 2, 1, 0)) < 0)
      {
        printf("main: failed to create TCP server socket error!\r\n");
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

static void wifi_cb(uint8_t u8_msg_type, void *pv_msg)
{
  switch (u8_msg_type)
  {
  case M2M_WIFI_RESP_SCAN_DONE:
  {
    wifi8_m2m_scan_done_t *pstr_info = (wifi8_m2m_scan_done_t *)pv_msg;
    scan_request_index = 0;
    if (pstr_info->u8_numof_ch >= 1)
    {
      wifi8_req_scan_result(&wifi8, scan_request_index);
      scan_request_index++;
    }
    else
    {
      wifi8_request_scan(&wifi8, M2M_WIFI_CH_ALL);
    }

    break;
  }
  case M2M_WIFI_RESP_SCAN_RESULT:
  {
    wifi8_m2m_wifiscan_result_t *pstr_scan_result = (wifi8_m2m_wifiscan_result_t *)pv_msg;
    uint16_t demo_ssid_len;
    uint16_t scan_ssid_len = strlen((char *)pstr_scan_result->au8ssid);

    printf("wifi_cb: [%d] SSID:%s\r\n", (uint16_t)scan_request_index, pstr_scan_result->au8ssid);

    num_found_ap = wifi8.ch_num;
    if (scan_ssid_len)
    {
      demo_ssid_len = strlen((const char *)MAIN_WLAN_SSID);
      if ((demo_ssid_len == scan_ssid_len) &&
          (!memcmp(pstr_scan_result->au8ssid, (uint8_t *)MAIN_WLAN_SSID, demo_ssid_len)))
      {
        printf("wifi_cb: found %s \r\n", MAIN_WLAN_SSID);
        wifi8_connect(&wifi8, MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID),
                      M2M_WIFI_SEC_WPA_PSK, MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
        break;
      }
    }

    if (scan_request_index < num_found_ap)
    {
      wifi8_req_scan_result(&wifi8, scan_request_index);
      scan_request_index++;
    }
    else
    {
      printf("wifi_cb: can not find AP %s\r\n", MAIN_WLAN_SSID);
      wifi8_request_scan(&wifi8, M2M_WIFI_CH_ALL);
    }

    break;
  }
  case M2M_WIFI_RESP_CON_STATE_CHANGED:
  {
    wifi8_m2m_wifi_state_changed_t *pstr_wifi_state = (wifi8_m2m_wifi_state_changed_t *)pv_msg;
    if (pstr_wifi_state->u8_curr_state == M2M_WIFI_CONNECTED)
    {
      printf("wifi_cb: connected\r\n");
    }
    else if (pstr_wifi_state->u8_curr_state == M2M_WIFI_DISCONNECTED)
    {
      printf("wifi_cb: disconnected\r\n");
      wifi_connected = M2M_WIFI_DISCONNECTED;

      wifi8_request_scan(&wifi8, M2M_WIFI_CH_ALL);
    }

    break;
  }
  case M2M_WIFI_REQ_DHCP_CONF:
  {
    volatile uint8_t *pu8ip_address = (uint8_t *)pv_msg;

    printf("wifi_cb: IP: %u.%u.%u.%u\r\n",
           (uint16_t)pu8ip_address[0], (uint16_t)pu8ip_address[1],
           (uint16_t)pu8ip_address[2], (uint16_t)pu8ip_address[3]);

    wifi_connected = M2M_WIFI_CONNECTED;
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
      printf("socket_cb: bind success!\r\n");
      delay_ms(500);
      wifi8_socket_listen(&wifi8, tcp_server_socket, 0);
    }
    else
    {
      printf("socket_cb: bind error!\r\n");
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
      printf("socket_cb: listen success!\r\n");
    }
    else
    {
      printf("socket_cb: listen error!\r\n");
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
      printf("socket_cb: accept success!\r\n");
      tcp_client_socket = pstr_accept->sock;
      wifi8_socket_receive(&wifi8, tcp_client_socket, gau8_socket_test_buffer, sizeof(gau8_socket_test_buffer), 0);
    }
    else
    {
      printf("socket_cb: accept error!\r\n");
      wifi8_socket_close(&wifi8, tcp_server_socket);
      tcp_server_socket = -1;
    }
  }
  break;
  case SOCKET_MSG_SEND:
  {
    printf("socket_cb: send success!\r\n");
    wifi8_socket_receive(&wifi8, tcp_client_socket, gau8_socket_test_buffer, sizeof(gau8_socket_test_buffer), 0);
  }
  break;
  case SOCKET_MSG_RECV:
  {
    wifi8_socket_recv_msg_t *pstr_recv = (wifi8_socket_recv_msg_t *)pv_msg;
    if (pstr_recv && pstr_recv->s16_buffer_size > 0)
    {
      printf("%s", pstr_recv->pu8_buffer);
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
      printf("socket_cb: close socket!\r\n");
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