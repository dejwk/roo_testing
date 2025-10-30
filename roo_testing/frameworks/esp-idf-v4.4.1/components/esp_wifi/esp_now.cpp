#include "esp_now.h"

#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

using namespace roo_testing_transducers;

extern "C" {

esp_err_t esp_now_init(void) { return ESP_OK; }

esp_err_t esp_now_deinit(void) { return ESP_OK; }

esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb) {
  FakeEsp32().esp_now().setRecvCb([cb](const uint8_t* mac, const uint8_t* data,
                                       size_t len) { cb(mac, data, len); });
  return ESP_OK;
}

esp_err_t esp_now_unregister_recv_cb(void) {
  FakeEsp32().esp_now().setRecvCb(nullptr);
  return ESP_OK;
}

esp_err_t esp_now_register_send_cb(esp_now_send_cb_t cb) {
  FakeEsp32().esp_now().setSendCb([cb](const uint8_t* mac, bool success) {
    cb(mac, success ? ESP_NOW_SEND_SUCCESS : ESP_NOW_SEND_FAIL);
  });
  return ESP_OK;
}

esp_err_t esp_now_unregister_send_cb(void) {
  FakeEsp32().esp_now().setSendCb(nullptr);
  return ESP_OK;
}

esp_err_t esp_now_add_peer(const esp_now_peer_info_t *peer) {
    return ESP_OK;
}

esp_err_t esp_now_del_peer(const uint8_t *peer_addr) {
    return ESP_OK;
}

esp_err_t esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len) {
    return FakeEsp32().esp_now().send(peer_addr, data, len) ? ESP_OK : ESP_FAIL;
}

}  // extern "C"