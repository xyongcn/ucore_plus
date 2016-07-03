#include <pci.h>
#include <kio.h>
#include <slab.h>
#include <stddef.h>
#include <assert.h>

const static uint16_t VENDOR_ID_INVALID = 0xFFFF;

list_entry_t pci_device_info_list;

static void pci_scan_host_controllers();
static void pci_scan_device(uint8_t bus, uint8_t device);
static void pci_scan_bus(uint8_t bus);
static void pci_scan_check_function(struct pci_device_info *device_info);

void pci_get_device_info(
  struct pci_device_info *device_info,
  uint8_t bus, uint8_t device, uint8_t function
) {
  device_info->bus = bus;
  device_info->device = device;
  device_info->function = function;
  uint32_t temp;
  temp = pci_read_register(bus, device, function, 0);
  device_info->device_id = (uint16_t)(temp >> 16);
  device_info->vendor_id = (uint16_t)temp;
  temp = pci_read_register(bus, device, function, 2);
  device_info->class_code = temp >> 24;
  device_info->subclass = temp >> 16;
  device_info->prog_if = temp >> 8;
  device_info->revision = temp;
  temp = pci_read_register(bus, device, function, 3);
  device_info->header_type = temp >> 16;
}

void pci_get_device_base_address(
  struct pci_device_info *device_info,
  int index,
  void** address_store,
  uint32_t* length_store,
  bool* is_port_io_store,
  uint8_t* type_store
) {
  //TODO: address can be 64-bit with bar_type=0x02, in this case it it consumes
  //2 PCI registers.
  uint32_t pci_address = pci_make_address(
    device_info->bus, device_info->device, device_info->function, 4 + index);
  machine_word_t address = pci_read_register_direct(pci_address);
  if(address & 0x1) {
    (*is_port_io_store) = 1;
    (*address_store) = (void*)(address & ~0x03);
  }
  else {
    (*is_port_io_store) = 0;
    (*type_store) = (address >> 1) & 0x03;
    (*address_store) = (void*)(address & ~0x0F);
    pci_write_register_direct(pci_address, 0xFFFFFFFF);
    uint32_t temp = pci_read_register_direct(pci_address);
    pci_write_register_direct(pci_address, (uint32_t)address);
    (*length_store) = (~(temp & ~0x0F)) + 1;
    if((*type_store) != 0x00)
      panic("TODO: 64-bit PCI address mapping is unsupported.");
  }
}

void pci_device_enable_bus_mastering(struct pci_device_info *device_info) {
  uint32_t pci_address = pci_make_address(
    device_info->bus, device_info->device, device_info->function, 1);
  uint32_t status_and_command = pci_read_register_direct(pci_address);
  status_and_command |= 0x04;
  pci_write_register_direct(pci_address, status_and_command);
}

uint8_t pci_device_get_interrupt_line(struct pci_device_info *device_info) {
  uint32_t pci_address = pci_make_address(
    device_info->bus, device_info->device, device_info->function, 1);
  return pci_read_register_direct(pci_address);
}

static void pci_scan_check_function(struct pci_device_info *device_info) {
  kprintf("PCI : Found device class = 0x%02x, subclass = 0x%02x,"
    "prog_if = 0x%02x, rev 0x%02x\n",
    (uint32_t)device_info->class_code, (uint32_t)device_info->subclass,
    (uint32_t)device_info->prog_if, (uint32_t)device_info->revision);
  kprintf("    device_id:0x%04x, vendor:0x%04x\n",
    (uint32_t)device_info->device_id, (uint32_t)device_info->vendor_id);
  list_add(&pci_device_info_list, &device_info->list_entry);
  if(device_info->class_code == 0x06 && device_info->subclass == 0x04) {
    //TODO: Recursive scan is untested, it may contains bug.
    uint8_t secondary_bus = pci_read_register(device_info->bus, device_info->device, device_info->function, 6) >> 8;
    pci_scan_bus(secondary_bus);
  }
}

static void pci_scan_device(uint8_t bus, uint8_t device) {
  struct pci_device_info *device_info = kmalloc(sizeof(struct pci_device_info));
  pci_get_device_info(device_info, bus, device, 0);
  if(device_info->vendor_id == VENDOR_ID_INVALID) {
    kfree(device_info);
    return;
  }
  pci_scan_check_function(device_info);
  if((device_info->header_type & 0x80) != 0) {
    for(uint8_t function = 1; function < 8; function++) {
      struct pci_device_info *device_info_extra = kmalloc(sizeof(struct pci_device_info));
      pci_get_device_info(device_info_extra, bus, device, function);
      if(device_info_extra->vendor_id != VENDOR_ID_INVALID) {
        pci_scan_check_function(device_info_extra);
      }
      else {
        kfree(device_info_extra);
      }
    }
  }
}

static void pci_scan_bus(uint8_t bus) {
  for(uint8_t device = 0; device < 32; device++) {
    pci_scan_device(bus, device);
  }
}

static void pci_scan_host_controllers() {
  uint8_t header_type = pci_read_register(0, 0, 0, 3) >> 16;
  if((header_type & 0x80) == 0) {
    kprintf("PCI : Found host controller 0\n");
    pci_scan_bus(0);
  }
  else {
    for(uint8_t function = 0; function < 8; function++) {
      struct pci_device_info device_info;
      pci_get_device_info(&device_info, 0, 0, function);
      if(device_info.vendor_id != 0xFFFF) {
        kprintf("PCI : Found host controller %d\n",(uint32_t)function);
        pci_scan_bus(function);
      }
    }
  }
}

void pci_init() {
  list_init(&pci_device_info_list);
  pci_scan_host_controllers();
}
