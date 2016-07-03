#include <types.h>
#include <slab.h>
#include <stdlib.h>
#include <string.h>
#include <list.h>
#include <ethernet.h>
#include <stddef.h>
#include <assert.h>

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

void ethernet_add_driver(struct ethernet_driver* driver)
{
  list_add(&ethernet_driver_list, &driver->list_entry);
}
