#ifndef __KERN_DRIVER_PCI_H__
#define __KERN_DRIVER_PCI_H__

#include <types.h>
#include <arch.h>
#include <list.h>

const static uint16_t PCI_ADDRESS_REGISTER = 0xCF8;
const static uint16_t PCI_DATA_REGISTER = 0xCFC;

struct pci_device_info {
  uint8_t bus;
  uint8_t device;
  uint8_t function;
  uint16_t device_id;
  uint16_t vendor_id;
  uint8_t class_code;
  uint8_t subclass;
  uint8_t prog_if;
  uint8_t revision;
  uint8_t header_type;
  list_entry_t list_entry;
};

extern list_entry_t pci_device_info_list;

static inline __attribute__((always_inline))
uint32_t pci_make_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg);

static inline __attribute__((always_inline))
uint32_t pci_read_register_direct(uint32_t address);

static inline __attribute__((always_inline))
void pci_write_register_direct(uint32_t address, uint32_t data);

static inline __attribute__((always_inline))
uint32_t pci_read_register(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg);

static inline __attribute__((always_inline))
void pci_write_register(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg, uint32_t data);

void pci_init();
void pci_get_device_info(
  struct pci_device_info *device_info,
  uint8_t bus, uint8_t device, uint8_t function
);

void pci_get_device_base_address(
  struct pci_device_info *device_info,
  int index,
  void** address_store,
  uint32_t* length_store,
  bool* is_port_io_store,
  uint8_t* type_store
  //TODO: prefetch flag store.s
);

void pci_device_enable_bus_mastering(struct pci_device_info *device_info);
uint8_t pci_device_get_interrupt_line(struct pci_device_info *device_info);

static inline __attribute__((always_inline))
uint32_t pci_make_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg)
{
  uint32_t address = 0x80000000;
  address |= ((uint32_t)bus) << 16;
  address |= ((uint32_t)device & 0x1F) << 11;
  address |= ((uint32_t)function & 0x07) << 8;
  address |= ((uint32_t)reg & 0x3F) << 2;
  return address;
}

static inline __attribute__((always_inline))
uint32_t pci_read_register_direct(uint32_t address)
{
  outl(PCI_ADDRESS_REGISTER, address);
  return inl(PCI_DATA_REGISTER);
}

static inline __attribute__((always_inline))
void pci_write_register_direct(uint32_t address, uint32_t data)
{
  outl(PCI_ADDRESS_REGISTER, address);
  outl(PCI_DATA_REGISTER, data);
}

static inline __attribute__((always_inline))
uint32_t pci_read_register(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg)
{
  uint32_t address = pci_make_address(bus, device, function, reg);
  return pci_read_register_direct(address);
}

static inline __attribute__((always_inline))
void pci_write_register(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg, uint32_t data)
{
  uint32_t address = pci_make_address(bus, device, function, reg);
  return pci_write_register_direct(address, data);
}

#endif /* __KERN_DRIVER_PCI_H__ */
