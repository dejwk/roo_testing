#include "lwip/ip_addr.h"
#include "lwip/netif.h"

extern "C" {

// NetworkManager clears lwIP's DNS cache when its emulated interface state
// changes. Linux owns resolver caching in host builds, so there is no local
// cache to clear.
void dns_clear_cache(void) {}

// Socket traffic uses the host kernel rather than an in-process lwIP stack.
// Keep lwIP's interface registry empty until the emulator grows an explicit
// Linux-interface adapter. This makes interface-dependent helpers degrade in
// the same way as other unsupported hardware-facing behavior.
#if !LWIP_SINGLE_NETIF
struct netif *netif_list = nullptr;
#endif
struct netif *netif_default = nullptr;

// NetworkUDP uses lwIP's canonical all-zero address while parsing packets.
// The host does not link lwIP's IP core, so provide that immutable data symbol
// alongside the other compatibility globals.
const ip_addr_t ip_addr_any = IPADDR4_INIT(IPADDR_ANY);

}  // extern "C"
