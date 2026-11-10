#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ================= RELAY GPIO SETUP =================
#define RELAY_1 18 // Light 1
#define RELAY_2 19 // Light 2
#define RELAY_3 21 // Fan
#define RELAY_4 22 // TV

// Active HIGH logic: 1 = ON (HIGH), 0 = OFF (LOW)
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

// Fixed Lock on Channel 1 (EMO Sender ke sath exact match)
#define FIXED_WIFI_CHANNEL 1

void setRelay(uint8_t device, uint8_t state) {
    uint8_t pin = 0;
    switch (device) {
        case 1: pin = RELAY_1; break;
        case 2: pin = RELAY_2; break;
        case 3: pin = RELAY_3; break;
        case 4: pin = RELAY_4; break;
        default: return;
    }

    digitalWrite(pin, (state == 1) ? RELAY_ON : RELAY_OFF);
    Serial.printf("[ACTION] Dev: %d -> %s (Pin %d)\n", device, (state == 1) ? "ON" : "OFF", pin);
}

// Receive callback (Compatible with both Core v2.x and v3.x)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
#endif
    if (len >= 2) {
        uint8_t dev = incomingData[0];
        uint8_t st  = incomingData[1];
        setRelay(dev, st);
    }
}

void setup() {
    Serial.begin(115200);

    // Boot par sabhi relays ko seedha OFF state par rakho
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(RELAY_3, OUTPUT);
    pinMode(RELAY_4, OUTPUT);

    digitalWrite(RELAY_1, RELAY_OFF);
    digitalWrite(RELAY_2, RELAY_OFF);
    digitalWrite(RELAY_3, RELAY_OFF);
    digitalWrite(RELAY_4, RELAY_OFF);

    // Wi-Fi Station Mode aur Channel 1 Lock
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(FIXED_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW Init Failed");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
    Serial.printf("[SYSTEM] Fast Receiver Locked on Channel %d!\n", FIXED_WIFI_CHANNEL);
}

void loop() {
    // Instant response loop
    delay(10);
}
