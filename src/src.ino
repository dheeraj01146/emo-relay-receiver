#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ================= RELAY GPIO SETUP =================
#define RELAY_1 18 // Light 1
#define RELAY_2 19 // Light 2
#define RELAY_3 21 // Fan
#define RELAY_4 22 // TV

// Active-LOW Relays: LOW hone par ON, HIGH hone par OFF
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

typedef struct struct_message {
    uint8_t device;
    uint8_t state;
} struct_message;

struct_message incomingPacket;

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
    Serial.printf("[RELAY] Device %d -> %s (Pin %d)\n", device, (state == 1) ? "ON" : "OFF", pin);
}

// Data Receive Callback
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    Serial.printf("[ESP-NOW RECV] Packet Len: %d\n", len);
    
    if (len == sizeof(struct_message)) {
        memcpy(&incomingPacket, incomingData, sizeof(struct_message));
        setRelay(incomingPacket.device, incomingPacket.state);
    } else if (len >= 2) {
        uint8_t dev = incomingData[0];
        uint8_t st  = incomingData[1];
        setRelay(dev, st);
    }
}

void setup() {
    Serial.begin(115200);

    // Boot hote hi Relays ko OFF (HIGH) rakho
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(RELAY_3, OUTPUT);
    pinMode(RELAY_4, OUTPUT);

    digitalWrite(RELAY_1, RELAY_OFF);
    digitalWrite(RELAY_2, RELAY_OFF);
    digitalWrite(RELAY_3, RELAY_OFF);
    digitalWrite(RELAY_4, RELAY_OFF);

    // Station Mode shuru karo
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW Init Failed");
        return;
    }

    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    Serial.println("[SYSTEM] Universal ESP-NOW Receiver Active & Scanning!");
}

// Background Channel Hopping: Kisi bhi channel (1 se 13) ka packet pakad lega
void loop() {
    static uint8_t current_channel = 1;
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
    
    current_channel++;
    if (current_channel > 13) {
        current_channel = 1;
    }
    
    delay(40);
}
