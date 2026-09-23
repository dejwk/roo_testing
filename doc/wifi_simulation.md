# Wi-Fi station simulation

Access points expose advertised authentication through `AccessPoint::setAuthMode`.
The ESP-IDF shim preserves this mode in scan records. Connected events report
negotiated authentication instead: WPA/WPA2 defaults to WPA2, WPA2/WPA3 defaults
to WPA3, and single-mode APs retain their mode. The host SDK configuration
advertises WPA3 SAE support so clients can use their normal capability checks.
This models connection behavior, not SAE cryptography or radio propagation.

## Scripted negotiated authentication

Use an optional per-attempt mode to exercise fallback or deliberately invalid
association reports without intercepting ESP-IDF symbols:

```cpp
using namespace roo_testing_transducers::wifi;
ConnectionAttempt attempt;
attempt.negotiated_auth_mode = AUTH_WPA2_PSK;
environment.queueConnectionAttempt(attempt);
```

The mode affects only that attempt's connected event. It neither changes the
AP's advertised mode nor persists into subsequent attempts. Leaving it unset
uses the default negotiation described above. Existing attempt outcomes and
timing settings continue to apply, including credential checks for automatic
attempts.

## DNS configuration

`esp_netif_set_dns_info` rejects unspecified IPv4 (`0.0.0.0`) and IPv6 (`::`)
addresses with `ESP_ERR_ESP_NETIF_INVALID_PARAMS`, preserving the old value.
Valid IPv4 and IPv6 DNS addresses are supported.

For a station DHCP-client interface, stop DHCP before calling
`esp_netif_set_ip_info`. Resetting IP settings clears primary and secondary DNS;
starting DHCP also clears these slots. Both operations preserve fallback DNS.
Do not use the DNS setter with a zero address as a clearing operation.
