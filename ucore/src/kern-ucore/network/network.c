#include <network.h>
#include <ethernet.h>
#include <network/e1000.h>

void network_init()
{
  ethernet_init();
}
