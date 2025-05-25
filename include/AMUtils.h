#include <Arduino.h>
#include <time.h>

void printLocalTime() {
#ifdef ESP8266
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
#else
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Не удалось получить время");
        return;
    }
#endif

    char buffer[50];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    Serial.print(buffer);
}