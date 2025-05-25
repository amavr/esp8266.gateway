#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <Preferences.h>

#define NS "Coord Config"

const int CAPTIVA_BEG = 1;
const int CAPTIVA_END = 2;
const int CHECK_WIFI_BEG = 3;
const int CHECK_WIFI_END = 4;
const int HAS_WIFI_BEG_TICK = 5;
const int HAS_WIFI_END_TICK = 6;

typedef void (*ConnectorCallbackFunc)(int);

static DNSServer dnsServer;
static ESP8266WebServer webServer(80);
Preferences prefs;

const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <title>WiFi Configuration</title>
    <style>
        body {font-family: 'Segoe UI', sans-serif;}
        span {width: 80px;display: inline-block;text-align: right;margin-right: 8px;}
        input {border-radius: 4px;border-width: 1px;height: 26px;}
        div {margin-bottom: 10px;}
    </style>
</head>
<body>
    <h1>Configure WiFi</h1>
    <form action="/wifi" method="POST">
        <div><span>SSID</span><input type="text" name="ssid" value="xxx"></div>
        <div><span>Password</span><input type="text" name="pass" value="xxx"></div>
        <div><span>&nbsp;</span><input type="submit" value="Connect"></div>
    </form>
</body>
</html>)rawliteral";

struct WiFiParams {
    String ssid = "";
    String pass = "";
};
WiFiParams wifi_params;

bool hasWiFi(ConnectorCallbackFunc callback = NULL) {
    bool result = false;
    if (WiFi.status() == WL_CONNECTED) {
        result = true;
    } else {
        prefs.begin(NS);
        wifi_params.ssid = prefs.getString("ssid");
        wifi_params.pass = prefs.getString("pass");
        prefs.end();
        Serial.println(wifi_params.ssid + "/" + wifi_params.pass);

        WiFi.disconnect();
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifi_params.ssid, wifi_params.pass, 0);

        unsigned long timeExit = millis() + 5000;
        while ((WiFi.status() != WL_CONNECTED) && (millis() < timeExit)) {
            if (callback != NULL) {
                callback(HAS_WIFI_BEG_TICK);
            }
            delay(300);
            if (callback != NULL) {
                callback(HAS_WIFI_END_TICK);
            }
            delay(700);
            if (callback == NULL) {
                Serial.print(".");
            }
        }
        Serial.println("");
        result = WiFi.status() == WL_CONNECTED;
    }
    return result;
}

volatile bool no_config = true;

// обработчик сабмита формы с новыми параметрами подключения к WiFi
void onWiFiRequest() {
    wifi_params.ssid = webServer.arg("ssid");
    wifi_params.pass = webServer.arg("pass");

    prefs.begin(NS);
    prefs.putString("ssid", wifi_params.ssid);
    prefs.putString("pass", wifi_params.pass);
    prefs.end();

    no_config = false;
}

void onRootRequest() {
    webServer.send(200, "text/html", html);
}

void stopCaptiva() {
    WiFi.softAPdisconnect();
    WiFi.disconnect();
    dnsServer.stop();
    webServer.stop();
}

void runCaptiva(ConnectorCallbackFunc callback = NULL) {
    if (callback != NULL) {
        callback(CAPTIVA_BEG);
    }
    IPAddress addrAP(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(addrAP, addrAP, subnet);
    WiFi.softAP("ESP Config");

    dnsServer.start(53, "*", addrAP);

    webServer.onNotFound(onRootRequest);
    webServer.on("/", HTTP_POST, onRootRequest);
    webServer.on("/wifi", HTTP_POST, onWiFiRequest);
    webServer.begin();

    unsigned long timeExit = millis() + 120000;

    no_config = true;
    while (no_config && millis() < timeExit) {
        dnsServer.processNextRequest();
        webServer.handleClient();
        // yield();
        delay(100);
    }

    stopCaptiva();
    if (callback != NULL) {
        callback(CAPTIVA_END);
    }
}

void checkWiFi(ConnectorCallbackFunc callback = NULL) {
    if (callback != NULL) {
        callback(CHECK_WIFI_BEG);
    }

    // бесконечный цикл, пока нет подключения к сети WiFi
    while (!hasWiFi(callback)) {
        runCaptiva(callback);
    }

    if (callback != NULL) {
        callback(CHECK_WIFI_END);
    }
}