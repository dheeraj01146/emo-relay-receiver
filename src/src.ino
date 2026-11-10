#include "emo_espnow.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <cstring>

#define TAG "EmoEspNow"

// Tera exact Receiver Board ka MAC Address
static uint8_t s_receiver_mac[6] = {0x68, 0x09, 0x47, 0x86, 0x26, 0xA0};
static bool s_espnow_initialized = false;

void InitEmoRelaySender() {
    if (s_espnow_initialized) return;

    if (esp_now_init() != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW Init Failed");
        return;
    }

    esp_now_peer_info_t peer_info = {};
    memcpy(peer_info.peer_addr, s_receiver_mac, 6);
    peer_info.channel = 0; // 0 matlab router ke live Wi-Fi channel par sync rahega
    peer_info.encrypt = false;

    if (esp_now_add_peer(&peer_info) != ESP_OK) {
        ESP_LOGW(TAG, "Peer already exists or add failed");
    } else {
        ESP_LOGI(TAG, "Receiver Peer Added: 68:09:47:86:26:A0");
    }

    s_espnow_initialized = true;
}

void SendRelayControl(uint8_t device, uint8_t state) {
    if (!s_espnow_initialized) {
        InitEmoRelaySender();
    }

    // 2-byte raw payload: [Device_ID, State]
    uint8_t payload[2] = {device, state};

    esp_err_t result = esp_now_send(s_receiver_mac, payload, sizeof(payload));
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Sent ESP-NOW -> Dev: %d, State: %d to Receiver", device, state);
    } else {
        ESP_LOGE(TAG, "ESP-NOW Send Error: %d", result);
    }
}
