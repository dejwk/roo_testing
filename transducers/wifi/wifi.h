#pragma once

#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <inttypes.h>
#include <memory>

#include "roo_testing/transducers/transducer.h"

namespace roo_testing_transducers {

namespace wifi {

class IpAddress {
 public:
  IpAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    addr_ = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) |
            ((uint32_t)d << 0);
  }

  bool operator==(const IpAddress& other) { return addr_ == other.addr_; }
  bool operator!=(const IpAddress& other) { return addr_ != other.addr_; }

 private:
  uint32_t addr_;
};

class MacAddress {
 public:
  struct hash {
    uint64_t operator()(const MacAddress& addr) const { return addr.addr_; }
  };

  MacAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) {
    addr_ = ((uint64_t)a << 40) | ((uint64_t)b << 32) | ((uint64_t)c << 24) |
            ((uint64_t)d << 16) | ((uint64_t)e << 8) | ((uint64_t)f << 0);
  }

  MacAddress(const uint8_t* addr)
      : MacAddress(addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]) {}

  bool operator==(const MacAddress& other) const {
    return addr_ == other.addr_;
  }

  bool operator!=(const MacAddress& other) const {
    return addr_ != other.addr_;
  }

  uint8_t get(int idx) const { return addr_ >> ((5 - idx) << 3); }

 private:
  friend class hash;

  uint64_t addr_;
};

class RSSI {
 public:
  constexpr RSSI(int8_t val) : val_(val < 0 ? val : 0) {}

  operator int8_t() const { return val_; }

 private:
  int8_t val_;
};

static constexpr RSSI kRssiVeryStrong = RSSI(-50);
static constexpr RSSI kRssiStrong = RSSI(-60);
static constexpr RSSI kRssiMedium = RSSI(-70);
static constexpr RSSI kRssiWeak = RSSI(-80);
static constexpr RSSI kRssiVeryWeak = RSSI(-90);

enum AuthMode {
  AUTH_OPEN = 0,
  AUTH_WEP,
  AUTH_WPA_PSK,
  AUTH_WPA2_PSK,
  AUTH_WPA_WPA2_PSK,
  AUTH_WPA2_ENTERPRISE
};

class Connection;
class Environment;

class AccessPoint {
 public:
  AccessPoint(const MacAddress& mac, const std::string& ssid)
      : mac_(mac),
        channel_(11),
        ssid_(ssid),
        visible_(true),
        auth_mode_(AUTH_OPEN),
        passwd_() {
    setRSSI(kRssiStrong);
  }

  const MacAddress& macAddress() const { return mac_; }

  const std::string& ssid() const { return ssid_; }
  const std::string& passwd() const { return passwd_; }

  bool isVisible() const { return visible_; }
  int channel() const { return channel_; }
  RSSI rssi() const { return rssi_(); }
  AuthMode auth_mode() const { return auth_mode_; }

  AccessPoint* setChannel(int channel) {
    channel_ = channel;
    return this;
  }

  AccessPoint* setRSSI(std::function<RSSI()> rssi) {
    rssi_ = rssi;
    return this;
  }

  AccessPoint* setRSSI(RSSI value) {
    return setRSSI([value]() -> RSSI { return value; });
  }

  AccessPoint* setAuthMode(AuthMode auth_mode) {
    auth_mode_ = auth_mode;
    return this;
  }

  AccessPoint* setSSID(const std::string& ssid) {
    ssid_ = ssid;
    return this;
  }

  AccessPoint* setPasswd(const std::string& passwd) {
    passwd_ = passwd;
    return this;
  }

  std::unique_ptr<Connection> createConnection(const MacAddress& mac_address);

 private:
  friend class Connection;
  friend class Environment;

  void setEnvironment(Environment* env);

  MacAddress mac_;
  int channel_;
  std::function<RSSI()> rssi_;

  std::string ssid_;
  bool visible_;

  AuthMode auth_mode_;
  std::string passwd_;

  std::unordered_map<MacAddress, Connection*, MacAddress::hash> connections_;

  Environment* env_;
};

enum ConnectionFailure {
  UNSPECIFIED_FAILURE = 1,
  AUTH_EXPIRED = 2,

  BEACON_TIMEOUT = 200,
  NO_AP_FOUND = 201,
  AUTH_FAILURE = 202,
  ASSOCIATION_FAILURE = 203,
  HANDSHAKE_TIMEOUT = 204,
  CONNECTION_FAILURE = 205,
  AP_TSF_RESET = 206,
  ROAMING = 207,
};

class ConnectionEventListener {
 public:
  virtual ~ConnectionEventListener() {}
  virtual void connected(const Connection& connection) {}
  virtual void disconnected(const Connection& connection) {}
  virtual void gotIP(const Connection& connection) {}
  virtual void connectionFailed(const Connection& connection,
                                ConnectionFailure failure) {}
};

class Environment {
 public:
  typedef std::unordered_map<MacAddress, std::unique_ptr<AccessPoint>,
                             MacAddress::hash>
      AccessPointMap;

  Environment() : event_listener_(nullptr) {}

  void addAccessPoint(std::unique_ptr<AccessPoint> access_point) {
    access_point->setEnvironment(this);
    aps_[access_point->macAddress()] = std::move(access_point);
  }

  const AccessPointMap& access_points() const { return aps_; }

  void setEventListener(ConnectionEventListener* listener) {
    event_listener_ = listener;
  }

 private:
  friend class Connection;

  void notifyConnected(const Connection& connection);

  void notifyDisconnected(const Connection& connection);

  void notifyGotIP(const Connection& connection);

  void notifyConnectionFailed(const Connection& connection,
                              ConnectionFailure failure);

  AccessPointMap aps_;

  ConnectionEventListener* event_listener_;
};

class Connection {
 public:
  ~Connection() { ap_->connections_.erase(mac_address()); }

  const MacAddress& mac_address() const { return mac_address_; }
  const AccessPoint& access_point() const { return *ap_; }

  void notifyConnected() { ap_->env_->notifyConnected(*this); }

  void notifyDisconnected() { ap_->env_->notifyDisconnected(*this); }

  void notifyGotIP(const Connection& connection) {
    ap_->env_->notifyGotIP(*this);
  }

  void notifyConnectionFailed(ConnectionFailure failure) {
    ap_->env_->notifyConnectionFailed(*this, failure);
  }

 private:
  friend class AccessPoint;

  Connection(AccessPoint* ap, MacAddress mac_address)
      : ap_(ap), mac_address_(mac_address) {}

  AccessPoint* ap_;
  MacAddress mac_address_;
};

inline std::unique_ptr<Connection> AccessPoint::createConnection(
    const MacAddress& mac_address) {
  auto conn = std::unique_ptr<Connection>(new Connection(this, mac_address));
  connections_[mac_address] = conn.get();
  return conn;
}

inline void AccessPoint::setEnvironment(Environment* env) { env_ = env; }

inline void Environment::notifyConnected(const Connection& connection) {
  if (event_listener_ == nullptr) return;
  event_listener_->connected(connection);
}

inline void Environment::notifyDisconnected(const Connection& connection) {
  if (event_listener_ == nullptr) return;
  event_listener_->disconnected(connection);
}

inline void Environment::notifyGotIP(const Connection& connection) {
  if (event_listener_ == nullptr) return;
  event_listener_->gotIP(connection);
}

inline void Environment::notifyConnectionFailed(const Connection& connection,
                                                ConnectionFailure failure) {
  if (event_listener_ == nullptr) return;
  event_listener_->connectionFailed(connection, failure);
}

}  // namespace wifi
}  // namespace roo_testing_transducers
