#pragma once

#include <cmath>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "roo_testing/transducers/transducer.h"

namespace roo_testing_transducers {

namespace wifi {

class IpAddress {
 public:
  IpAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    addr_ = a << 24 | b << 16 | c << 8 | d;
  }

  bool operator==(const IpAddress& other) { return addr_ == other.addr_; }

 private:
  uint32_t addr_;
};

class MacAddress {
 public:
  MacAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) {
    addr_ = a << 40 | b << 32 | c << 24 | d << 16 | e << 8 | f;
  }

  bool operator==(const MacAddress& other) { return addr_ == other.addr_; }

 private:
  uint64_t addr_;
};

class RSSI {
 public:
  static constexpr RSSI kVeryStrong = RSSI(-50);
  static constexpr RSSI kStrong = RSSI(-60);
  static constexpr RSSI kMedium = RSSI(-70);
  static constexpr RSSI kWeak = RSSI(-80);
  static constexpr RSSI kVeryWeak = RSSI(-90);

  constexpr RSSI(int8_t val) : val_(std::min(0, val)) {}

  operator int8_t() const { return val_; }

 private:
  int8_t val_;
};

enum AuthMode {
  AUTH_OPEN = 0,
  AUTH_WEP,
  AUTH_WPA_PSK,
  AUTH_WPA2_PSK,
  AUTH_WPA_WPA2_PSK,
  AUTH_WPA2_ENTERPRISE
};

class AccessPoint {
 public:
  AccessPoint(const MacAddress& mac, const std::string& ssid)
      : mac_(mac),
        channel_(11),
        ssid_(ssid),
        visible_(true),
        auth_mode_(AUTH_OPEN),
        passwd_() {
    setRSSI(RSSI::kStrong);
  }

  const MacAddress& macAddress() const { return mac_; }

  const std::string& ssid() const { return ssid_; }
  const std::string& passwd() const { return passwd_; }

  int channel() const { return channel_; }
  RSSI rssi() const { return rssi_(); }

  AccessPoint* setChannel(int channel) {
    channel_ = channel;
    return this;
  }

  AccessPoint* setRSSI(std::function<RSSI()> rssi) {
    rssi_ = rssi;
    return this;
  }

  AccessPoint* void setRSSI(RSSI value) {
    return setRSSI([]() -> RSSI { return value; });
  }

  AccessPoint* setSSID(const std::string& ssid) {
    ssid_ = ssid;
    return this;
  }

  AccessPoint* setPasswd(const std::string& passwd) {
    passwd_ = passwd;
    return this;
  }

 private:
  MacAddress mac_;
  int channel_;
  std::function<RSSI()> rssi_;

  std::string ssid_;
  bool visible_;

  AuthMode auth_mode_;
  std::string passwd_;
};

class Environment {
 public:
  Environment() {}

  void addAccessPoint(std::unique_ptr<AccessPoint> access_point) {
    aps_[access_point_->mac()] = std::move(access_point);
  }

  const std::map<MacAddress, std::unique_ptr<AccessPoint>>& access_points() const {
    return aps_;
  }

 private:
  std::map<MacAddress, std::unique_ptr<AccessPoint>> aps_;
};

}  // namespace wifi

// enum WifiScanType {
//   WIFI_SCAN_TYPE_ACTIVE = 0,
//   WIFI_SCAN_TYPE_PASSIVE,
// };

// /** Range of active scan times per channel */
// struct WifiScanTime {
//   uint32_t min; /**< minimum active scan time per channel, units: millisecond
//   */ uint32_t max; /**< maximum active scan time per channel, units:
//   millisecond,
//                    values above 1500ms may cause station to disconnect from
//                    AP and are not recommended.  */
// };

// struct WifiScanConfig {
//   uint8_t* ssid;          /**< SSID of AP */
//   uint8_t* bssid;         /**< MAC address of AP */
//   uint8_t channel;        /**< channel, scan the specific channel */
//   bool show_hidden;       /**< enable to scan AP whose SSID is hidden */
//   WifiScanType scan_type; /**< scan type, active or passive */
//   WifiScanTime scan_time; /**< scan time per channel */
// };

// enum WifiOperationResult {
//   OK = 0,
//   ERR_WIFI_NOT_INIT = 0x3000 + 1,
//   ERR_WIFI_NOT_STARTED = 0x3000 + 2,
//   ERR_WIFI_NOT_STOPPED =
//       0x3000 + 3,              /*!< WiFi driver was not stopped by wifi_stop
//       */
//   ERR_WIFI_IF = 0x3000 + 4,    /*!< WiFi interface error */
//   ERR_WIFI_MODE = 0x3000 + 5,  /*!< WiFi mode error */
//   ERR_WIFI_STATE = 0x3000 + 6, /*!< WiFi internal state error */
//   ERR_WIFI_CONN =
//       0x3000 +
//       7, /*!< WiFi internal control block of station or soft-AP error */
//   ERR_WIFI_NVS = 0x3000 + 8,       /*!< WiFi internal NVS module error */
//   ERR_WIFI_MAC = 0x3000 + 9,       /*!< MAC address is invalid */
//   ERR_WIFI_SSID = 0x3000 + 10,     /*!< SSID is invalid */
//   ERR_WIFI_PASSWORD = 0x3000 + 11, /*!< Password is invalid */
//   ERR_WIFI_TIMEOUT = 0x3000 + 12,  /*!< Timeout error */
//   ERR_WIFI_WAKE_FAIL =
//       0x3000 + 13, /*!< WiFi is in sleep state=RF closed, and wakeup fail */
//   ERR_WIFI_WOULD_BLOCK = 0x3000 + 14, /*!< The caller would block */
//   ERR_WIFI_NOT_CONNECT = 0x3000 + 15, /*!< Station still in disconnect status
//   */
// };

// enum WifiCountryPolicy {
//   WIFI_COUNTRY_POLICY_AUTO, /**< Country policy is auto, use the country info
//   of
//                                AP to which the station is connected */
//   WIFI_COUNTRY_POLICY_MANUAL, /**< Country policy is manual, always use the
//                                  configured country info */
// };

// struct WifiCountry {
//   char cc[3];               /**< country code string */
//   uint8_t schan;            /**< start channel */
//   uint8_t nchan;            /**< total channel number */
//   int8_t max_tx_power;      /**< maximum tx power */
//   WifiCountryPolicy policy; /**< country policy */
// };

// enum WifiSecondaryChannel {
//   WIFI_SECOND_CHAN_NONE = 0, /**< the channel width is HT20 */
//   WIFI_SECOND_CHAN_ABOVE,    /**< the channel width is HT40 and the secondary
//                                 channel is above the primary channel */
//   WIFI_SECOND_CHAN_BELOW,    /**< the channel width is HT40 and the secondary
//                                 channel is below the primary channel */
// };

// enum WifiAuthMode {
//   WIFI_AUTH_OPEN = 0,        /**< authenticate mode : open */
//   WIFI_AUTH_WEP,             /**< authenticate mode : WEP */
//   WIFI_AUTH_WPA_PSK,         /**< authenticate mode : WPA_PSK */
//   WIFI_AUTH_WPA2_PSK,        /**< authenticate mode : WPA2_PSK */
//   WIFI_AUTH_WPA_WPA2_PSK,    /**< authenticate mode : WPA_WPA2_PSK */
//   WIFI_AUTH_WPA2_ENTERPRISE, /**< authenticate mode : WPA2_ENTERPRISE */
//   WIFI_AUTH_MAX
// };

// enum WifiCipherType {
//   WIFI_CIPHER_TYPE_NONE = 0,  /**< the cipher type is none */
//   WIFI_CIPHER_TYPE_WEP40,     /**< the cipher type is WEP40 */
//   WIFI_CIPHER_TYPE_WEP104,    /**< the cipher type is WEP104 */
//   WIFI_CIPHER_TYPE_TKIP,      /**< the cipher type is TKIP */
//   WIFI_CIPHER_TYPE_CCMP,      /**< the cipher type is CCMP */
//   WIFI_CIPHER_TYPE_TKIP_CCMP, /**< the cipher type is TKIP and CCMP */
//   WIFI_CIPHER_TYPE_UNKNOWN,   /**< the cipher type is unknown */
// };

// enum WifiAntenna {
//   WIFI_ANT_ANT0, /**< WiFi antenna 0 */
//   WIFI_ANT_ANT1, /**< WiFi antenna 1 */
// };

// struct WifiApRecord {
//   uint8_t bssid[6];               /**< MAC address of AP */
//   uint8_t ssid[33];               /**< SSID of AP */
//   uint8_t primary;                /**< channel of AP */
//   WifiSecondaryChannel second;    /**< secondary channel of AP */
//   int8_t rssi;                    /**< signal strength of AP */
//   WifiAuthMode authmode;          /**< authmode of AP */
//   WifiCipherType pairwise_cipher; /**< pairwise cipher of AP */
//   WifiCipherType group_cipher;    /**< group cipher of AP */
//   WifiAntenna ant;                /**< antenna used to receive beacon from AP
//   */ uint32_t
//       phy_11b : 1; /**< bit: 0 flag to identify if 11b mode is enabled or not
//       */
//   uint32_t
//       phy_11g : 1; /**< bit: 1 flag to identify if 11g mode is enabled or not
//       */
//   uint32_t
//       phy_11n : 1; /**< bit: 2 flag to identify if 11n mode is enabled or not
//       */
//   uint32_t
//       phy_lr : 1; /**< bit: 3 flag to identify if low rate is enabled or not
//       */
//   uint32_t wps : 1; /**< bit: 4 flag to identify if WPS is supported or not
//   */ uint32_t reserved : 27; /**< bit: 5..31 reserved */ WifiCountry country;
//   /**< country information of AP */
// };

// enum WifiScanMethod {
//   WIFI_FAST_SCAN =
//       0, /**< Do fast scan, scan will end after find SSID match AP */
//   WIFI_ALL_CHANNEL_SCAN, /**< All channel scan, scan will end after scan all
//   the
//                             channel */
// };

// enum WifiSortMethod {
//   WIFI_CONNECT_AP_BY_SIGNAL = 0, /**< Sort match AP in scan list by RSSI */
//   WIFI_CONNECT_AP_BY_SECURITY, /**< Sort match AP in scan list by security
//   mode
//                                 */
// };

// struct WifiFastScanThreshold {
//   int8_t rssi; /**< The minimum rssi to accept in the fast scan mode */
//   WifiAuthMode
//       authmode; /**< The weakest authmode to accept in the fast scan mode */
// };

// /** STA configuration settings */
// struct WifiStaConfig {
//   uint8_t ssid[32];           /**< SSID of target AP*/
//   uint8_t password[64];       /**< password of target AP*/
//   WifiScanMethod scan_method; /**< do all channel scan or fast scan */
//   bool bssid_set;   /**< whether set MAC address of target AP or not.
//   Generally,
//                       station_config.bssid_set needs to be 0; and it needs to
//                       be 1 only when users need to check the MAC address of
//                       the AP.*/
//   uint8_t bssid[6]; /**< MAC address of target AP*/
//   uint8_t channel;  /**< channel of target AP. Set to 1~13 to scan starting
//   from
//                        the specified channel before connecting to AP. If the
//                        channel of AP is unknown, set it to 0.*/
//   uint16_t listen_interval;   /**< Listen interval for ESP32 station to
//   receive
//                                  beacon when WIFI_PS_MAX_MODEM is set. Units:
//                                  AP beacon intervals. Defaults to 3 if set to
//                                  0. */
//   WifiSortMethod sort_method; /**< sort the connect AP in the list by rssi or
//                                  security mode */
//   WifiFastScanThreshold
//       threshold; /**< When scan_method is set to WIFI_FAST_SCAN, only APs
//       which
//                     have an auth mode that is more secure than the selected
//                     auth mode and a signal stronger than the minimum RSSI
//                     will be used. */
// };

// class WifiRatioEventListener {
//  public:
//   void connected(const WifiApRecord& result) = 0;
//   void connectError(WifiOperationResult result) = 0;
//   void disconnected() = 0;
//   void authModeChanged() = 0;
//   void gotIP() = 0;
//   void lostIP() = 0;
// };

// class WifiRadio : public Transducer {
//  public:
//   struct ScanResult {
//     std::vector<WifiApRecord> networks;
//     WifiOperationResult result;
//   };

//   WifiRadio() {}

//   // Synchronous. Will be called in a separate thread.
//   virtual ScanResult scan(const WifiScanConfig& config) = 0;

//   // Asynchronous. The implementaion should make sure that connected() or
//   // connectError() is called on the listener when completed.
//   virtual WifiOperationResult connect(const WifiStaConfig& config) = 0;

//   virtual void setEventListener(std::unique_ptr<WifiRadioEentListener>) = 0;
// };

// class NullWifiRadio : public WifiRadio {
//  public:
//   ScanResult scan(const WifiScanConfig& config) override {
//     return ScanResult{
//         .result = OK,
//     };
//   }

//   WifiOperationResult connect(const WifiStaConfig& config) override {
//     return ERR_WIFI_SSID;
//   }

//   void setEventListener(std::unique_ptr<WifiRadioEentListener>) override {}
// };

// class SimpleWifiRadio : public WifiRadio {
//  public:
//   void setEventListener(std::unique_ptr<WifiRadioEentListener> listener)
//   override {
//     listener_ = std::move(listener);
//   }

//   void notifyConnected(const WifiApRecord& result) {
//     listener_->connected(result);
//   }

//  private:
//   std::unique_ptr<WifiRadioEventListener> listener_;
// };

}  // namespace roo_testing_transducers
