#include <WiFi.h>
#include <esp_now.h>

// ================= RELAY GPIO SETUP =================
#define RELAY_1 18 // Light 1
#define RELAY_2 19 // Light 2
#define RELAY_3 21 // Fan
#define RELAY_4 22 // TV

// Active-LOW Relays: LOW par ON, HIGH par OFF
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

// ESP-NOW Data Receive Callback
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(struct_message)) {
        memcpy(&incomingPacket, incomingData, sizeof(struct_message));
        Serial.printf("[ESP-NOW] Device: %d, State: %d\n", incomingPacket.device, incomingPacket.state);
        setRelay(incomingPacket.device, incomingPacket.state);
    } else if (len >= 2) {
        uint8_t dev = incomingData[0];
        uint8_t st  = incomingData[1];
        Serial.printf("[ESP-NOW RAW] Device: %d, State: %d\n", dev, st);
        setRelay(dev, st);
    }
}

void setup() {
    Serial.begin(115200);

    // Relays ko pehle hi OFF (HIGH) set karo taaki boot hote hi click na kare
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(RELAY_3, OUTPUT);
    pinMode(RELAY_4, OUTPUT);

    digitalWrite(RELAY_1, RELAY_OFF);
    digitalWrite(RELAY_2, RELAY_OFF);
    digitalWrite(RELAY_3, RELAY_OFF);
    digitalWrite(RELAY_4, RELAY_OFF);

    // Wi-Fi ko sirf Station mode me rakho (ESP-NOW ke liye zaroori hai)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.println("[SYSTEM] Initializing ESP-NOW Receiver...");

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW Init Failed!");
        return;
    }

    // Callback register karo
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    Serial.println("[ESP-NOW] Receiver Ready & Listening!");
}



void loop() {
    // ESP-NOW pura background interrupt par chalta hai, loop ko shant rakho
    delay(100);
}
