#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ================= WIFI & MQTT SETTINGS =================
const char* WIFI_SSID     = "JIO SMART HOME";
const char* WIFI_PASSWORD = "Dhiman66666"; // Apna WiFi password daalo
const char* MQTT_BROKER   = "broker.emqx.io";
const int   MQTT_PORT     = 1883;

// Topics matching your Web Dashboard
const char* TOPIC_RELAY_SUB = "dheeraj_emo_hub_9921/control/relay";
const char* TOPIC_STATE_PUB = "dheeraj_emo_hub_9921/status/relay";
const char* CLIENT_ID       = "emo_hybrid_receiver_9921";

// ================= RELAY GPIO CONFIGURATION =================
#define RELAY_1 16 // Light 1
#define RELAY_2 17 // Light 2
#define RELAY_3 18 // Fan
#define RELAY_4 19 // TV

// Standard 4-channel relay modules are ACTIVE LOW
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ================= ESP-NOW PACKET STRUCTURE =================
typedef struct struct_message {
    uint8_t device;
    uint8_t state;
} struct_message;

struct_message incomingPacket;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Relay execution helper
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
    Serial.printf("[RELAY] Device: %d -> %s (Pin: %d)\n", device, (state == 1) ? "ON" : "OFF", pin);

    // Send state feedback to Web/Cloud
    if (mqttClient.connected()) {
        StaticJsonDocument<128> doc;
        doc["id"] = device;
        doc["state"] = state;
        char buffer[128];
        serializeJson(doc, buffer);
        mqttClient.publish(TOPIC_STATE_PUB, buffer);
    }
}

// ================= ESP-NOW CALLBACK =================
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(struct_message)) {
        memcpy(&incomingPacket, incomingData, sizeof(struct_message));
        Serial.printf("[ESP-NOW] Voice Command Recv -> Dev: %d, State: %d\n", incomingPacket.device, incomingPacket.state);
        setRelay(incomingPacket.device, incomingPacket.state);
    } else if (len >= 2) {
        uint8_t dev = incomingData[0];
        uint8_t st  = incomingData[1];
        Serial.printf("[ESP-NOW RAW] Dev: %d, State: %d\n", dev, st);
        setRelay(dev, st);
    }
}

// ================= MQTT CALLBACK =================
void mqttCallback(char* topic, byte* message, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        msg += (char)message[i];
    }
    Serial.printf("[MQTT] Message on %s: %s\n", topic, msg.c_str());

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (!error) {
        if (doc.containsKey("id") && doc.containsKey("state")) {
            uint8_t dev = doc["id"];
            uint8_t st  = doc["state"];
            setRelay(dev, st);
        }
    }
}

void reconnectMQTT() {
    if (!mqttClient.connected()) {
        Serial.print("[MQTT] Connecting to broker...");
        if (mqttClient.connect(CLIENT_ID)) {
            Serial.println(" Connected!");
            mqttClient.subscribe(TOPIC_RELAY_SUB);
        } else {
            Serial.printf(" Failed, rc=%d. Retrying next cycle.\n", mqttClient.state());
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize Relay Pins (Default: OFF)
    pinMode(RELAY_1, OUTPUT); digitalWrite(RELAY_1, RELAY_OFF);
    pinMode(RELAY_2, OUTPUT); digitalWrite(RELAY_2, RELAY_OFF);
    pinMode(RELAY_3, OUTPUT); digitalWrite(RELAY_3, RELAY_OFF);
    pinMode(RELAY_4, OUTPUT); digitalWrite(RELAY_4, RELAY_OFF);

    // Wi-Fi STA mode enables both standard Wi-Fi & ESP-NOW together
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[WIFI] Connecting to router");
    
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 8000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connected! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n[WIFI] Wi-Fi link skipped/failed. Running in Offline ESP-NOW Mode!");
    }

    // Init ESP-NOW
    if (esp_now_init() == ESP_OK) {
        Serial.println("[ESP-NOW] Engine Initialized Successfully.");
        esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    } else {
        Serial.println("[ESP-NOW] Init Failed!");
    }

    // MQTT Init
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        static unsigned long lastMqttRetry = 0;
        if (!mqttClient.connected()) {
            if (millis() - lastMqttRetry > 5000) {
                lastMqttRetry = millis();
                reconnectMQTT();
            }
        } else {
            mqttClient.loop();
        }
    }
    delay(10);
}
