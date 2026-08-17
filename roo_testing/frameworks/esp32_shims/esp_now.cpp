#include "esp_now.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>
#include <vector>

#include "esp_mac.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {

std::mutex g_mutex;
std::vector<esp_now_peer_info_t> g_peers;
esp_now_recv_cb_t g_recv_callback = nullptr;
esp_now_send_cb_t g_send_callback = nullptr;

bool SameMac(const uint8_t* first, const uint8_t* second) {
  return memcmp(first, second, ESP_NOW_ETH_ALEN) == 0;
}

}  // namespace

extern "C" {

esp_err_t esp_now_init(void) { return ESP_OK; }

esp_err_t esp_now_deinit(void) {
  FakeEsp32().esp_now().setRecvCb(nullptr);
  FakeEsp32().esp_now().setSendCb(nullptr);
  std::lock_guard<std::mutex> lock(g_mutex);
  g_peers.clear();
  g_recv_callback = nullptr;
  g_send_callback = nullptr;
  return ESP_OK;
}

esp_err_t esp_now_get_version(uint32_t* version) {
  if (version == nullptr) return ESP_ERR_ESPNOW_ARG;
  *version = 2;
  return ESP_OK;
}

esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t callback) {
  if (callback == nullptr) return ESP_ERR_ESPNOW_ARG;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_recv_callback = callback;
  }
  FakeEsp32().esp_now().setRecvCb(
      [](const uint8_t* source, const uint8_t* data, size_t size) {
        esp_now_recv_cb_t callback_copy;
        {
          std::lock_guard<std::mutex> lock(g_mutex);
          callback_copy = g_recv_callback;
        }
        if (callback_copy == nullptr) return;
        uint8_t destination[ESP_NOW_ETH_ALEN];
        esp_read_mac(destination, ESP_MAC_WIFI_STA);
        esp_now_recv_info_t info = {
            .src_addr = const_cast<uint8_t*>(source),
            .des_addr = destination,
            .rx_ctrl = nullptr,
        };
        callback_copy(&info, data, static_cast<int>(size));
      });
  return ESP_OK;
}

esp_err_t esp_now_unregister_recv_cb(void) {
  FakeEsp32().esp_now().setRecvCb(nullptr);
  std::lock_guard<std::mutex> lock(g_mutex);
  g_recv_callback = nullptr;
  return ESP_OK;
}

esp_err_t esp_now_register_send_cb(esp_now_send_cb_t callback) {
  if (callback == nullptr) return ESP_ERR_ESPNOW_ARG;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_send_callback = callback;
  }
  FakeEsp32().esp_now().setSendCb([](const uint8_t* destination, bool success) {
    esp_now_send_cb_t callback_copy;
    {
      std::lock_guard<std::mutex> lock(g_mutex);
      callback_copy = g_send_callback;
    }
    if (callback_copy == nullptr) return;
    uint8_t source[ESP_NOW_ETH_ALEN];
    esp_read_mac(source, ESP_MAC_WIFI_STA);
    esp_now_send_info_t info = {};
    info.des_addr = const_cast<uint8_t*>(destination);
    info.src_addr = source;
    info.ifidx = WIFI_IF_STA;
    info.tx_status = success ? WIFI_SEND_SUCCESS : WIFI_SEND_FAIL;
    callback_copy(&info,
                  success ? ESP_NOW_SEND_SUCCESS : ESP_NOW_SEND_FAIL);
  });
  return ESP_OK;
}

esp_err_t esp_now_unregister_send_cb(void) {
  FakeEsp32().esp_now().setSendCb(nullptr);
  std::lock_guard<std::mutex> lock(g_mutex);
  g_send_callback = nullptr;
  return ESP_OK;
}

esp_err_t esp_now_send(const uint8_t* peer, const uint8_t* data, size_t size) {
  if (data == nullptr || size == 0 || size > ESP_NOW_MAX_DATA_LEN_V2) {
    return ESP_ERR_ESPNOW_ARG;
  }
  static constexpr uint8_t broadcast[ESP_NOW_ETH_ALEN] = {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  return FakeEsp32().esp_now().send(peer == nullptr ? broadcast : peer, data,
                                    size)
             ? ESP_OK
             : ESP_ERR_ESPNOW_NOT_FOUND;
}

esp_err_t esp_now_add_peer(const esp_now_peer_info_t* peer) {
  if (peer == nullptr) return ESP_ERR_ESPNOW_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  if (std::any_of(g_peers.begin(), g_peers.end(), [&](const auto& current) {
        return SameMac(current.peer_addr, peer->peer_addr);
      })) {
    return ESP_ERR_ESPNOW_EXIST;
  }
  g_peers.push_back(*peer);
  return ESP_OK;
}

esp_err_t esp_now_del_peer(const uint8_t* address) {
  if (address == nullptr) return ESP_ERR_ESPNOW_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  const auto before = g_peers.size();
  g_peers.erase(std::remove_if(g_peers.begin(), g_peers.end(),
                               [&](const auto& peer) {
                                 return SameMac(peer.peer_addr, address);
                               }),
                g_peers.end());
  return g_peers.size() == before ? ESP_ERR_ESPNOW_NOT_FOUND : ESP_OK;
}

esp_err_t esp_now_mod_peer(const esp_now_peer_info_t* peer) {
  if (peer == nullptr) return ESP_ERR_ESPNOW_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  for (auto& current : g_peers) {
    if (SameMac(current.peer_addr, peer->peer_addr)) {
      current = *peer;
      return ESP_OK;
    }
  }
  return ESP_ERR_ESPNOW_NOT_FOUND;
}

bool esp_now_is_peer_exist(const uint8_t* address) {
  if (address == nullptr) return false;
  std::lock_guard<std::mutex> lock(g_mutex);
  return std::any_of(g_peers.begin(), g_peers.end(), [&](const auto& peer) {
    return SameMac(peer.peer_addr, address);
  });
}

esp_err_t esp_now_get_peer(const uint8_t* address, esp_now_peer_info_t* peer) {
  if (address == nullptr || peer == nullptr) return ESP_ERR_ESPNOW_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  for (const auto& current : g_peers) {
    if (SameMac(current.peer_addr, address)) {
      *peer = current;
      return ESP_OK;
    }
  }
  return ESP_ERR_ESPNOW_NOT_FOUND;
}

esp_err_t esp_now_get_peer_num(esp_now_peer_num_t* number) {
  if (number == nullptr) return ESP_ERR_ESPNOW_ARG;
  std::lock_guard<std::mutex> lock(g_mutex);
  number->total_num = static_cast<int>(g_peers.size());
  number->encrypt_num = static_cast<int>(
      std::count_if(g_peers.begin(), g_peers.end(),
                    [](const auto& peer) { return peer.encrypt; }));
  return ESP_OK;
}

esp_err_t esp_now_set_pmk(const uint8_t* key) {
  return key == nullptr ? ESP_ERR_ESPNOW_ARG : ESP_OK;
}
esp_err_t esp_now_set_wake_window(uint16_t) { return ESP_OK; }

}  // extern "C"
