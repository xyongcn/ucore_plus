#ifndef __KERN_NETWORK_INPUT_THREAD_H__
#define __KERN_NETWORK_INPUT_THREAD_H__

#include <types.h>
#include <sem.h>

struct netif;
struct pbuf;

void network_input_thread_init();
void network_input_thread_notify(struct netif *netif, struct pbuf *pbuf);
void network_input_thread_main(void* args);

#endif /* __KERN_NETWORK_INPUT_THREAD_H__ */
