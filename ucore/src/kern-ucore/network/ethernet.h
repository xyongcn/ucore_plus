#ifndef __KERN_NETWORK_ETHERNET_H__
#define __KERN_NETWORK_ETHERNET_H__

#include <types.h>
#include <list.h>

typedef char mac_address_t[6];
typedef char ether_type_t[2];

struct ethernet_frame
{
  mac_address_t source;
  mac_address_t target;
  char data[];
};

struct ethernet_driver;

typedef void (*ethernet_driver_send_handler_t)(
  struct ethernet_driver* driver, uint16_t length, char* data);
typedef void (*ethernet_driver_receive_handler_t)(
  struct ethernet_driver* driver, uint16_t length, char* data);
typedef void (*ethernet_driver_get_mac_address_handler_t)(
  struct ethernet_driver* driver, char* address);

struct ethernet_driver
{
  ethernet_driver_send_handler_t send_handler;
  ethernet_driver_receive_handler_t receive_handler;
  ethernet_driver_get_mac_address_handler_t get_mac_address_handler;
  void* private_data;
  list_entry_t list_entry;
};

int ethernet_driver_send_data(struct ethernet_driver* driver,
  mac_address_t target, ether_type_t ether_type, uint16_t data_length, char* data);
int ethernet_send_data(mac_address_t target, ether_type_t ether_type, uint16_t data_length, char* data);

void ethernet_init();
void ethernet_add_driver(struct ethernet_driver* driver);

#endif /* __KERN_NETWORK_ETHERNET_H__ */
