#include <types.h>
#include <net_byteorder.h>

//TODO: We are assuming little-endian here!

static void _swap_char(char* c1, char* c2)
{
  char temp;
  temp = *c1;
  (*c1) = *c2;
  (*c2) = temp;
}

uint16_t htons(uint16_t val)
{
  char* temp = &val;
  _swap_char(&temp[0], &temp[1]);
  return val;
}

uint16_t ntohs(uint16_t val)
{
  return htons(val);
}

uint32_t htonl(uint32_t val)
{
  char* temp = &val;
  _swap_char(&temp[0], &temp[3]);
  _swap_char(&temp[1], &temp[2]);
  return val;
}

uint32_t ntohl(uint32_t val)
{
  return htonl(val);
}
