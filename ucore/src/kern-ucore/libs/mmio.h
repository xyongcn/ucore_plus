#ifndef __LIBS_MMIO_H__
#define __LIBS_MMIO_H__

/*
 * MMIO (Memory-mapped I/O) library. What those functions do are simply
 * memory access, with the "volatile" keyword to prevent any possible
 * optimization, which may lead to misbehaviour for hardware registers.
 */

static uint8_t mmio_read8 (volatile uint8_t* address)
{
  return *address;
}

static uint16_t mmio_read16 (volatile uint16_t* address)
{
  return *address;
}

static uint32_t mmio_read32 (volatile uint32_t* address)
{
  return *address;
}

static uint64_t mmio_read64 (volatile uint64_t* address)
{
  return *address;
}

static void mmio_write8(volatile uint8_t* address, uint8_t value)
{
  (*address) = value;
}

static void mmio_write16(volatile uint16_t* address, uint16_t value)
{
  (*address) = value;
}

static void mmio_write32(volatile uint32_t* address, uint32_t value)
{
  (*address) = value;
}

static void mmio_write64(volatile uint64_t* address, uint64_t value)
{
  (*address) = value;
}

#endif /* __LIBS_MMIO_H__ */
