#ifndef __KERN_NETWORK_NETWORK_H__
#define __KERN_NETWORK_NETWORK_H__

#include <ethernet.h>

static const mac_address_t MAC_ADDRESS_BROADCAST =
  {'\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF'};

static const ether_type_t ETHER_TYPE_IPV4 = {'\x08', '\x00'};

void network_init();

#endif /* __KERN_NETWORK_NETWORK_H__ */
