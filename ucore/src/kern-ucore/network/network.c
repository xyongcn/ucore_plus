#include <network.h>
#include <ethernet.h>
#include <network/e1000.h>
#include "lwip/init.h"

void network_init()
{
  ethernet_init();
  lwip_init();
}
