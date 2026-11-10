#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ================= RELAY GPIO SETUP =================
#define RELAY_1 18 // Light 1
#define RELAY_2 19 // Light 2
#define RELAY_3 21 // Fan
#define RELAY_4 22 // TV

// Active HIGH: 1 = ON (HIGH), 0 = OFF (LOW)
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

typedef struct struct_message {
    uint8_t device;
    uint8_t state;
} struct_message;

// Current state cache to prevent redundant switching
uint8_t current_state[5] = {0, 0, 0, 0, 0};

void setRelay(uint8_t device, uint8_t state) {
    uint8_t pin = 0;
    switch (device) {
        case 1: pin = RELAY_1; break;
        case 2: pin = RELAY_2; break;
        case 3: pin = RELAY_3; break;
        case 4: pin = RELAY_4; break;
        default: return;
    }

    // Set pin state directly
    digitalWrite(pin, (state == 1) ? RELAY_ON : RELAY_OFF);
    current_state[device] = state;
    Serial.printf("[ACTION] Device %d -> %s (Pin %d)\n", device, (state == 1) ? "ON" : "OFF", pin);
}

// Data Receive Callback
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
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

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW Init Failed");
        return;
    }

    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    Serial.println("[SYSTEM] Receiver Active & Stable!");
}

void loop() {
    static unsigned long last_hop = 0;
    static uint8_t current_channel = 1;

    // Scan cycle ko thoda relax rakho taaki radio interrupt drop na ho
    if (millis() - last_hop > 80) {
        last_hop = millis();
        esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
        current_channel++;
        if (current_channel > 13) current_channel = 1;
    }
    delay(5);
}
