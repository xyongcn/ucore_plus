#include <arch.h>
#include <mmio.h>
#include <slab.h>
#include <list.h>
#include <pci.h>
#include <stddef.h>
#include <kio.h>
#include <pmm.h>
#include <ethernet.h>
#include <assert.h>
#include <picirq.h>
#include <string.h>
#include <interrupt_manager.h>
#include "e1000.h"

void e1000_write_command(struct e1000_driver* driver, uint16_t p_address, uint32_t p_value)
{
  if(driver->bar_type == 0) {
    mmio_write32(driver->mem_base + p_address, p_value);
  }
  else {
    outl(driver->io_base, p_address);
    outl(driver->io_base + 4, p_value);
  }
}

uint32_t e1000_read_command(struct e1000_driver* driver, uint16_t p_address)
{
  if(driver->bar_type == 0) {
    return mmio_read32(driver->mem_base + p_address);
  }
  else {
    outl(driver->io_base, p_address);
    return inl(driver->io_base + 4);
  }
}

bool e1000_detect_EEPROM(struct e1000_driver* driver)
{
  uint32_t val = 0;
  e1000_write_command(driver, REG_EEPROM, 0x1);

  for(int i = 0; i < 1000 && !driver->eerprom_exists; i++)
  {
    val = e1000_read_command(driver, REG_EEPROM);
    if(val & 0x10)
      driver->eerprom_exists = 1;
    else
      driver->eerprom_exists = 0;
  }
  return driver->eerprom_exists;
}

uint32_t e1000_EEPROM_read(struct e1000_driver* driver, uint8_t addr)
{
	uint16_t data = 0;
	uint32_t tmp = 0;
  if(driver->eerprom_exists)
  {
    e1000_write_command(driver, REG_EEPROM, (1) | ((uint32_t)(addr) << 8));
  	while( !((tmp = e1000_read_command(driver, REG_EEPROM)) & (1 << 4)) );
  }
  else
  {
    e1000_write_command(driver, REG_EEPROM, (1) | ((uint32_t)(addr) << 2) );
    while( !((tmp = e1000_read_command(driver, REG_EEPROM)) & (1 << 1)) );
  }
	data = (uint16_t)((tmp >> 16) & 0xFFFF);
	return data;
}

bool e1000_read_MAC_address(struct e1000_driver* driver)
{
  if (driver->eerprom_exists)
  {
      uint32_t temp;
      temp = e1000_EEPROM_read(driver, 0);
      driver->mac[0] = temp &0xff;
      driver->mac[1] = temp >> 8;
      temp = e1000_EEPROM_read(driver, 1);
      driver->mac[2] = temp &0xff;
      driver->mac[3] = temp >> 8;
      temp = e1000_EEPROM_read(driver, 2);
      driver->mac[4] = temp &0xff;
      driver->mac[5] = temp >> 8;
  }
  else
  {
    uint8_t * mem_base_mac_8 = (uint8_t *) (driver->mem_base+0x5400);
    uint32_t * mem_base_mac_32 = (uint32_t *) (driver->mem_base+0x5400);
    if(mem_base_mac_32[0] != 0) {
      for(int i = 0; i < 6; i++) {
        driver->mac[i] = mem_base_mac_8[i];
      }
    }
    else return 0;
  }
  return 1;
}

void e1000_rxinit(struct e1000_driver* driver)
{
    uint8_t *ptr;
    struct e1000_rx_desc *descs;

    // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
    // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones

    ptr = (uint8_t *)kmalloc(sizeof(struct e1000_rx_desc)*E1000_NUM_RX_DESC + 16);

    descs = (struct e1000_rx_desc *)ptr;
    for(int i = 0; i < E1000_NUM_RX_DESC; i++)
    {
      driver->rx_descs[i] = (struct e1000_rx_desc *)((uint8_t *)descs + i*16);
      driver->rx_descs[i]->addr = PADDR(kmalloc(8192 + 16));
      driver->rx_descs[i]->status = 0;
    }

    ptr = PADDR(ptr);
    e1000_write_command(driver, REG_TXDESCLO, (uint32_t)((uint64_t)ptr >> 32) );
    e1000_write_command(driver, REG_TXDESCHI, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));

    e1000_write_command(driver, REG_RXDESCLO, (uint64_t)ptr);
    e1000_write_command(driver, REG_RXDESCHI, 0);

    e1000_write_command(driver, REG_RXDESCLEN, E1000_NUM_RX_DESC * 16);

    e1000_write_command(driver, REG_RXDESCHEAD, 0);
    e1000_write_command(driver, REG_RXDESCTAIL, E1000_NUM_RX_DESC-1);
    driver->rx_cur = 0;
    e1000_write_command(driver, REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_2048);

}

void e1000_txinit(struct e1000_driver* driver)
{
    uintptr_t ptr;
    struct e1000_tx_desc *descs;
    // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
    // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
    //TODO...
    ptr = (uintptr_t)(kmalloc(sizeof(struct e1000_tx_desc)*E1000_NUM_TX_DESC + 16));

    descs = (struct e1000_tx_desc *)ptr;
    for(int i = 0; i < E1000_NUM_TX_DESC; i++)
    {
        driver->tx_descs[i] = &descs[i];
        driver->tx_descs[i]->addr = 0;
        driver->tx_descs[i]->cmd = 0;
        driver->tx_descs[i]->status = TSTA_DD;
    }

    ptr = PADDR(ptr);
    e1000_write_command(driver, REG_TXDESCHI, (uint32_t)((uint64_t)ptr >> 32));
    e1000_write_command(driver, REG_TXDESCLO, (uint32_t)((uint64_t)ptr & 0xFFFFFFFF));


    //now setup total length of descriptors
    e1000_write_command(driver, REG_TXDESCLEN, E1000_NUM_TX_DESC * 16);


    //setup numbers
    e1000_write_command(driver, REG_TXDESCHEAD, 0);
    e1000_write_command(driver, REG_TXDESCTAIL, 0);
    driver->tx_cur = 0;
    e1000_write_command(driver, REG_TCTRL,  TCTL_EN
        | TCTL_PSP
        | (15 << TCTL_CT_SHIFT)
        | (64 << TCTL_COLD_SHIFT)
        | TCTL_RTLC);

    // This line of code overrides the one before it but I left both to highlight that the previous one works with e1000 cards, but for the e1000e cards
    // you should set the TCTRL register as follows. For detailed description of each bit, please refer to the Intel Manual.
    // In the case of I217 and 82577LM packets will not be sent if the TCTRL is not configured using the following bits.
    e1000_write_command(driver, REG_TCTRL,  0b0110000000000111111000011111010);
    e1000_write_command(driver, REG_TIPG,  0x0060200A);
}

void e1000_enable_interrupt(struct e1000_driver* driver)
{
  e1000_write_command(driver, REG_IMASK ,0xFFFFFFFF);
//  e1000_write_command(driver, 0x00d8 ,0xFFFFFFFF);
  //kprintf("IntMask = %lx\n", e1000_read_command(driver, REG_IMASK));
  //kprintf("Thro = %lx\n", e1000_read_command(driver, 0x00C4));
    //e1000_write_command(driver, REG_IMASK ,0x1F6DC);
    //e1000_write_command(driver, REG_IMASK ,0xff & ~4);
    //e1000_read_command(driver, 0xc0);
}

void e1000_linkup(struct e1000_driver *driver)
{
	uint32_t val;
	val = e1000_read_command(driver, REG_CTRL);
	e1000_write_command(driver, REG_CTRL, val | ECTRL_SLU);
}

int e1000_send_packet(struct e1000_driver* driver, const char *p_data, uint16_t p_len)
{
    driver->tx_descs[driver->tx_cur]->addr = PADDR(p_data);
    driver->tx_descs[driver->tx_cur]->length = p_len;
    driver->tx_descs[driver->tx_cur]->cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
    driver->tx_descs[driver->tx_cur]->status = 0;
    uint8_t old_cur = driver->tx_cur;
    driver->tx_cur = (driver->tx_cur + 1) % E1000_NUM_TX_DESC;
    e1000_write_command(driver, REG_TXDESCTAIL, driver->tx_cur);
    while(!(driver->tx_descs[old_cur]->status & 0xff));
    return 0;
}

//TODO: Now only the interrupt of one device is handled.
static struct e1000_driver *__driver = NULL;

bool e1000_interrupt_handler(struct trapframe *tf)
{
  //TODO: Check all e1000 network devices.
  if(__driver == NULL) return;
  uint32_t status = e1000_read_command(__driver, 0xc0);
  if(status & 0x04) {
    e1000_linkup(__driver);
    return 1;
  }
  else if(status & 0x10) {
    //TODO: I don't know what this means.
    return 1;
  }
  else if(status & 0x80) {
    //To avoid deadlock, receive will not be handled here, OS will be notified
    //and a kernel thread will handle the input.
    __driver->ethernet_driver->receive_notifier(__driver);
    return 1;
  }
  return status != 0;
}

void e1000_handle_receive(struct e1000_driver* driver, uint16_t *length, uint8_t **data)
{
  uint16_t old_cur;
  if((driver->rx_descs[driver->rx_cur]->status & 0x1)) {
    uint8_t *buf = KADDR((uintptr_t)driver->rx_descs[driver->rx_cur]->addr);
    uint16_t len = driver->rx_descs[driver->rx_cur]->length;
    char* copied_data = kmalloc(len);
    memcpy(copied_data, buf, len);
    *data = copied_data;
    *length = len;
    driver->rx_descs[driver->rx_cur]->status = 0;
    old_cur = driver->rx_cur;
    driver->rx_cur = (driver->rx_cur + 1) % E1000_NUM_RX_DESC;
    e1000_write_command(driver, REG_RXDESCTAIL, old_cur);
  }
  else {
    *length = 0;
    *data = NULL;
  }
}

void e1000_create(struct e1000_driver* driver, struct pci_device_info* device_info)
{
  bool is_port_io;
  uint8_t bar_type;
  void* address;
  uint32_t length;
  pci_get_device_base_address(device_info, 0, &address, &length, &is_port_io, &driver->bar_type);

  //TODO: This is only tested for x86 and amd64
#ifndef __UCORE_64__
  void* address_kern = address;
#else
  void* address_kern = address + KERNBASE;
#endif
  pmm_mmio_map_direct(boot_pgdir, address, address_kern, length, PTE_P | PTE_W);
  driver->bar_type = is_port_io;
  driver->io_base = (uint16_t)address;
  driver->mem_base = (uintptr_t)address_kern;
  // Enable bus mastering
  pci_device_enable_bus_mastering(device_info);
  e1000_detect_EEPROM(driver);
  e1000_read_MAC_address(driver);
  kprintf("    MAC Address is ");
  for(int i = 0; i < 6; i++) {
    if(i != 0) kprintf(":");
    kprintf("%02x", driver->mac[i]);
  }
  kprintf("\n");
  kprintf("    Physics address range : %p-%p\n", address, address + length);

	uint32_t flags = e1000_read_command(driver, REG_RCTRL);
	e1000_write_command(driver, REG_RCTRL, flags | RCTL_EN);
  e1000_linkup(driver);
  //Clear rx and tx buffer
  for(int i = 0; i < 0x80; i++)
    e1000_write_command(driver, 0x5200 + i*4, 0);
  e1000_enable_interrupt(driver);
  uint8_t irq = pci_device_get_interrupt_line(device_info);
  kprintf("    IRQ : %d\n", irq);
  interrupt_manager_register_handler(IRQ_OFFSET + irq, e1000_interrupt_handler);
  interrupt_manager_register_handler(46, e1000_interrupt_handler);
#ifndef ARCH_AMD64
  pic_enable(irq);
#else
  irq_enable(irq);
#endif
  e1000_rxinit(driver);
  e1000_txinit(driver);
}

void e1000_ethernet_driver_send_handler(
  struct ethernet_driver* driver, uint16_t length, uint8_t *data
) {
  struct e1000_driver *e1000_driver =
    (struct e1000_driver*)driver->private_data;
  //TODO: adjust the parameter order of e1000_send_packet
  e1000_send_packet(e1000_driver, data, length);
}

void e1000_ethernet_driver_receive_handler(
  struct ethernet_driver* driver, uint16_t *length, uint8_t **data
) {
  struct e1000_driver *e1000_driver =
    (struct e1000_driver*)driver->private_data;
  //TODO: adjust the parameter order of e1000_send_packet
  e1000_handle_receive(e1000_driver, length, data);
}

void e1000_ethernet_driver_get_mac_address_handler(
  struct ethernet_driver* driver, uint8_t* mac_store
) {
  struct e1000_driver *e1000_driver =
    (struct e1000_driver*)driver->private_data;
  memcpy(mac_store, e1000_driver->mac, 6);
}

struct ethernet_driver* e1000_ethernet_driver_create(
   struct pci_device_info* device
) {
  struct e1000_driver* e1000_driver = kmalloc(sizeof(struct e1000_driver));
  struct ethernet_driver* ethernet_driver = kmalloc(sizeof(struct ethernet_driver));
  ethernet_driver->private_data = e1000_driver;
  e1000_driver->ethernet_driver = ethernet_driver;
  e1000_create(e1000_driver, device);
  ethernet_driver->send_handler = e1000_ethernet_driver_send_handler;
  ethernet_driver->receive_handler = e1000_ethernet_driver_receive_handler;
  ethernet_driver->get_mac_address_handler =
    e1000_ethernet_driver_get_mac_address_handler;
  return ethernet_driver;
}

void e1000_init()
{
  for(list_entry_t* i = list_next(&pci_device_info_list);
  i != &pci_device_info_list; i = list_next(i)) {
    struct pci_device_info* device = container_of(i, struct pci_device_info, list_entry);
    if(device->vendor_id == INTEL_VEND) {
      char* device_name = "";
      switch(device->device_id) {
        case E1000_DEV:
          device_name = "Intel e1000";
          break;
        case E1000_I217:
          device_name = "Intel I217";
          break;
        case E1000_82577LM:
          device_name = "Intel 82577lm";
          break;
      }
      if(device_name[0] == '\0') continue;
      kprintf("E1000 : Found %s on PCI Bus 0x%02x Dev 0x%02x Func 0x%02x\n",
        device_name, device->bus, device->device, device->function);
      struct ethernet_driver *driver = e1000_ethernet_driver_create(device);
      __driver = driver->private_data;
      ethernet_add_driver(driver);
    }
  }
}
