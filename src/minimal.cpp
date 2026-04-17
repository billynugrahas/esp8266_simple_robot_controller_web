// RobotWebUI Minimal Example
// Demonstrates the simplest integration: connect WiFi + view system info
#include <RobotWebUI.h>

const char* WIFI_SSID = "YourSSID";
const char* WIFI_PASS = "YourPassword";

RobotWebUI ui;

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("[Demo] Minimal RobotWebUI example starting...");
    ui.begin(WIFI_SSID, WIFI_PASS);
}

void loop() {
    ui.loop();
}
