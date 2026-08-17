// NetworkManager clears lwIP's DNS cache when its emulated interface state
// changes. Linux owns resolver caching in host builds, so there is no local
// cache to clear.
extern "C" void dns_clear_cache(void) {}
