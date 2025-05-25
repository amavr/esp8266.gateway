#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <espnow.h>

#include "AMUtils.h"
#include "WiFiConnector.h"

#define WIFI_SSID "amavr-d"
#define WIFI_PASS "220666abba"
#define MQTT_HOST "srv2.clusterfly.ru"
#define MQTT_USER "user_feec9173"
#define MQTT_PASS "Caf2Hy2xLmf0M"
#define MQTT_CLI_ID "esp8266.gateway"
#define CHANNEL 4

#define LED_ON false
#define LED_OFF true

const char *wifi_ssid = "xxx";
const char *wifi_pass = "xxx";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct {
    uint8_t cmd;
    uint8_t prm;
} SensorData;

struct RegisteredDevice {
    uint8_t mac[6];
    String topic;
};

RegisteredDevice devices[6];
int device_count = 0;

void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
    if (len == sizeof(SensorData)) {
        SensorData received;
        memcpy(&received, incomingData, sizeof(received));

        Serial.print("received.cmd: ");
        Serial.print(received.cmd);
        Serial.print(" received.prm: ");
        Serial.println(received.prm);

        client.publish(String(String(MQTT_USER) + "/abc").c_str(), String(received.prm).c_str());
    }
}

// void connectToWiFi() {
//     WiFi.begin(wifi_ssid, wifi_pass);
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("Wi-Fi connected");
// }

void connector_cb(int whereCalled) {
    switch (whereCalled) {
        case CAPTIVA_BEG:
            Serial.println("BEG runCaptiva");
            digitalWrite(LED_BUILTIN, LED_ON);
            break;
        case CAPTIVA_END:
            digitalWrite(LED_BUILTIN, LED_OFF);
            Serial.println("END runCaptiva");
            break;
        case CHECK_WIFI_BEG:
            Serial.println("BEG checkWiFi");
            break;
        case CHECK_WIFI_END:
            Serial.println("END checkWiFi");
            break;
        case HAS_WIFI_BEG_TICK:
            digitalWrite(LED_BUILTIN, LED_ON);
            break;
        case HAS_WIFI_END_TICK:
            digitalWrite(LED_BUILTIN, LED_OFF);
            break;
    }
}

void connectToMQTT() {
    client.setServer(MQTT_HOST, 9991);
    while (!client.connect("esp8266.gateway", MQTT_USER, MQTT_PASS)) {
        checkWiFi(connector_cb);
    }
    Serial.println("MQTT connected");
}

void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_ON);

    Serial.println();
    Serial.println();
    Serial.println();
    checkWiFi(connector_cb);
    connectToMQTT();

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != 0) {
        Serial.println("ESP-NOW init error");
        return;
    }

    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, CHANNEL, NULL, 0);
    // wifi_set_channel(CHANNEL);
    esp_now_register_recv_cb(onDataRecv);
    digitalWrite(LED_BUILTIN, LED_OFF);
    Serial.println("ESP-NOW init success");
}

unsigned long lastTime = 0;
unsigned long delayCheckWiFi = 2000;  // send readings timer
unsigned long delayLedOff = 300;      // send readings timer
bool isLedOn = false;

void loop() {
    if (!client.connected()) connectToMQTT();
    client.loop();

    // проверить WiFi
    lastTime = millis();
    if ((millis() - lastTime) > delayCheckWiFi) {
        checkWiFi(connector_cb);
        // lastTime = millis();
    }
}