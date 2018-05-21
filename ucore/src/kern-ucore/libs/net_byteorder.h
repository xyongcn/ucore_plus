#ifndef __LIBS_NET_BYTEORDER_H__
#define __LIBS_NET_BYTEORDER_H__

#include <types.h>

uint16_t htons(uint16_t val);
uint16_t ntohs(uint16_t val);
uint32_t htonl(uint32_t val);
uint32_t ntohl(uint32_t val);

#endif /* __LIBS_NET_BYTEORDER_H__ */
