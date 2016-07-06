#include <types.h>
#include <slab.h>
#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <ethernet.h>
#include <stddef.h>
#include <assert.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "netif/etharp.h"
#include "netif/ppp/pppoe.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"

list_entry_t ethernet_driver_list;

int ethernet_driver_send_data(struct ethernet_driver* driver,
  mac_address_t target, ether_type_t ether_type, uint16_t data_length, char* data)
{
  uint16_t total_length = sizeof(mac_address_t) * 2 + sizeof(uint16_t) + data_length;
  char* raw_data = kmalloc(total_length);
  char* current_pos = raw_data;
  memcpy(current_pos, target, sizeof(mac_address_t));
  current_pos += sizeof(mac_address_t);
  mac_address_t source;
  driver->get_mac_address_handler(driver, source);
  memcpy(current_pos, source, sizeof(mac_address_t));
  current_pos += sizeof(mac_address_t);
  memcpy(current_pos, ether_type, sizeof(ether_type_t));
  current_pos += sizeof(ether_type_t);
  memcpy(current_pos, data, data_length);
  driver->send_handler(driver, total_length, raw_data);
  kfree(raw_data);
  //TODO: error handling
  return 0;
}

int ethernet_send_data(mac_address_t target, ether_type_t ether_type, uint16_t data_length, char* data)
{
  //TODO: need to handle different ethernet adapter.
  for(list_entry_t* i = list_next(&ethernet_driver_list);
  i != &ethernet_driver_list; i = list_next(i)) {
    struct ethernet_driver* driver = container_of(i, struct ethernet_driver, list_entry);
    ethernet_driver_send_data(driver, target, ether_type, data_length, data);
  }
  //TODO: error handling
  return 0;
}

void ethernet_init()
{
  list_init(&ethernet_driver_list);
}

static struct ethernet_driver *lwip_current_driver = NULL;

static err_t ethernet_lwip_low_level_output(struct netif *netif, struct pbuf *p)
{
  struct ethernet_driver *driver = (struct ethernet_driver*)netif->state;

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

  int total_length = 0;
  for (struct pbuf *q = p; q != NULL; q = q->next) {
    total_length += q->len;
  }
  char* buffer = kmalloc(total_length);
  char* current_pos = buffer;
  for (struct pbuf *q = p; q != NULL; q = q->next) {
    memcpy(current_pos, q->payload, q->len);
    current_pos += q->len;
  }
  driver->send_handler(driver, total_length, buffer);
  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t*)p->payload)[0] & 1) {
    /* broadcast or multicast packet*/
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

err_t ethernet_lwip_netif_init(struct netif *netif)
{
  static int current_netif = 0;
  assert(lwip_current_driver != NULL);

#if LWIP_NETIF_HOSTNAME
  netif->hostname = "ucore_device";
#endif

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->state = lwip_current_driver;
  netif->name[0] = 'e';
  netif->name[1] = 't';
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = ethernet_lwip_low_level_output;

  //ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

  /* initialize the hardware */
  //low_level_init(netif);
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  lwip_current_driver->get_mac_address_handler(lwip_current_driver, netif->hwaddr);

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, MLD6_ADD_MAC_FILTER);
  }
#endif

  return ERR_OK;
}

void ethernet_add_driver(struct ethernet_driver* driver)
{
  list_add(&ethernet_driver_list, &driver->list_entry);
  lwip_current_driver = driver;
  struct netif *netif = kmalloc(sizeof(struct netif));
  ip_addr_t ipaddr, netmask, gateway;
  IP4_ADDR(&gateway, 192,168,3,1);
  IP4_ADDR(&ipaddr, 192,168,3,100);
  IP4_ADDR(&netmask, 255,255,255,0);
  netif_add(netif, &ipaddr, &netmask, &gateway, 0, ethernet_lwip_netif_init, tcpip_input);
  netif_set_link_up(netif);
  netif_set_up(netif);
  int err = dhcp_start(netif);
  kprintf("dhcp_start err = %d\n", err);
}
