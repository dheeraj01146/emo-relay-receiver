#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ================= RELAY GPIO SETUP =================
#define RELAY_1 18 // Light 1
#define RELAY_2 19 // Light 2
#define RELAY_3 21 // Fan
#define RELAY_4 22 // TV

// Active HIGH logic: 1 = ON (HIGH), 0 = LOW
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

uint8_t current_channel = 6; // Default channel (Jio default)
unsigned long last_recv_time = 0;
unsigned long last_hop_time = 0;
bool is_locked = false;

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
    Serial.printf("[ACTION] Dev: %d -> %s (Pin %d) on Ch %d\n", device, (state == 1) ? "ON" : "OFF", pin, current_channel);
}

void setWifiChannel(uint8_t ch) {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
#endif
    if (len >= 2) {
        last_recv_time = millis();
        is_locked = true; // Lock ho gaya, ab scan nahi karega!

        uint8_t dev = incomingData[0];
        uint8_t st  = incomingData[1];
        setRelay(dev, st);
    }
}

void setup() {
    Serial.begin(115200);

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

    setWifiChannel(current_channel);

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW Init Failed");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
    Serial.printf("[SYSTEM] Smart Receiver Started on Channel %d\n", current_channel);
}

void loop() {
    // Agar packet mil chuka hai, toh kisi scanning ki zaroorat nahi hai (Zero Lag)
    // Lekin agar 10 second se EMO se koi link na mila ho, tabhi dheere-dheere channel switch karega
    if (!is_locked || (millis() - last_recv_time > 10000)) {
        if (millis() - last_hop_time > 300) { // Har 300ms me silent hop
            last_hop_time = millis();
            current_channel++;
            if (current_channel > 13) current_channel = 1;
            setWifiChannel(current_channel);
        }
    }

    delay(5);
}
