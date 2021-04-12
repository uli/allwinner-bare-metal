// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/timeouts.h"

#include "ccu.h"
#include "network.h"
#include "util.h"
#include "system.h"

#include <device/emac.h>

#include <string.h>

extern void emac_eth_send(void*, int);
extern int emac_eth_recv(uint8_t **);
extern void emac_free_pkt(void);

// used by the lib-h3 EMAC driver
uint8_t libh3_coherent_region[1048576]  __attribute__ ((section ("UNCACHED")));

static struct pbuf *eth_recv_pbuf(void)
{
  uint8_t *eth_data;
  int eth_data_count = emac_eth_recv(&eth_data);
  if (!eth_data_count)
    return NULL;

  struct pbuf* p = pbuf_alloc(PBUF_RAW, eth_data_count, PBUF_POOL);

  if(p != NULL)
    pbuf_take(p, eth_data, eth_data_count);

  emac_free_pkt();

  return p;
}

static err_t netif_output(struct netif *netif, struct pbuf *p)
{
  (void)netif;
  // XXX: This is a lot of copying. (emac_eth_send() copies the buffer, too.)
  uint8_t send_buffer[p->tot_len];
  pbuf_copy_partial(p, send_buffer, p->tot_len, 0);
  emac_eth_send(send_buffer, p->tot_len);

  return ERR_OK;
}

static void netif_status_callback(struct netif *netif)
{
  printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
}

extern int32_t hardware_get_mac_address(uint8_t *mac_address);

static err_t _netif_init(struct netif *netif)
{
  uint8_t mac[6];

  netif->linkoutput = netif_output;
  netif->output     = etharp_output;
  netif->mtu        = 1500;
  netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;

  // NB: This reads the MAC from the EMAC's registers and thus only works if
  // the EMAC is clocked and unresetted.
  hardware_get_mac_address(mac);
  //printf("==mac %02X %02X %02X %02X %02X %02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  SMEMCPY(netif->hwaddr, mac, ETH_HWADDR_LEN);
  netif->hwaddr_len = ETH_HWADDR_LEN;

  return ERR_OK;
}

struct netif netif_eth0 = {};

void network_task(void)
{
  struct pbuf *p = NULL;

  if (!netif_is_link_up(&netif_eth0))
    return;

  do {
#if 0	// XXX
    if(link_state_changed()) {
      if(link_is_up()) {
        netif_set_link_up(&netif_eth0);
      } else {
        netif_set_link_down(&netif_eth0);
      }
    }
#endif

    /* Check for received frames, feed them to lwIP */
    p = eth_recv_pbuf();

    if(p != NULL) {
      if(netif_eth0.input(p, &netif_eth0) != ERR_OK) {
        pbuf_free(p);
      }
    }

    sys_check_timeouts();	/* lwip timers */
  } while (p);
}

void network_if_start(void)
{
  // slow and/or blocking stuff

  // bring link up
  emac_start(1);
  printf("emac started\n");

  netif_set_link_up(&netif_eth0);
}

void network_if_stop(void)
{
  netif_set_link_down(&netif_eth0);
  emac_shutdown();
}

void network_init(void)
{
  // fast initialization bits that do not delay boot

  // turn on hardware
  BUS_CLK_GATING0 |= BIT(17);
  BUS_SOFT_RST0 |= BIT(17);
  emac_init();
  printf("emac inited\n");

  // initialize IP stack
  lwip_init();

  // create interface
  netif_add(&netif_eth0, IP4_ADDR_ANY, IP4_ADDR_ANY, IP4_ADDR_ANY, NULL, _netif_init, netif_input);
  netif_eth0.name[0] = 'e';
  netif_eth0.name[1] = 'n';
  netif_set_status_callback(&netif_eth0, netif_status_callback);
  netif_set_default(&netif_eth0);
  netif_set_up(&netif_eth0);
  netif_set_link_down(&netif_eth0);
}

u32_t sys_now(void)
{
  return sys_get_msec();
}
