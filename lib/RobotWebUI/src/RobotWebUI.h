#pragma once
#include "ITransport.h"
#include "WSProtocol.h"

class RobotWebUI {
public:
    // D-01: constructor takes no args, all config in begin()
    RobotWebUI();

    // D-01: single setup call. motorCount defaults to 2 (differential drive)
    void begin(const char* ssid, const char* password, int motorCount = 2);

    // Must be called in loop() for Arduino transport
    void loop();

    // D-02: typed push methods per sensor
    void pushDistance(float cm);
    void pushIR(bool detected);
    void pushOdometry(float x, float y, float heading);
    void pushBoolean(int index, bool state);

    // D-03: per-type callbacks with typed structs
    void onMotorCommand(void (*cb)(const MotorCmd&));
    void onWiFiCommand(void (*cb)(const WiFiCmd&));
    void onCoefficientCommand(void (*cb)(const CoefCmd&));

private:
    ITransport* _transport;
    WSProtocol _protocol;
    int _motorCount;

    // Callbacks
    void (*_motorCallback)(const MotorCmd&) = nullptr;
    void (*_wifiCallback)(const WiFiCmd&) = nullptr;
    void (*_coefCallback)(const CoefCmd&) = nullptr;

    // System info push timer
    unsigned long _lastSystemPush = 0;
    static const unsigned long SYSTEM_PUSH_INTERVAL = 3000; // 3 seconds

    // Motor safety timeout (MOTO-04 / D-03)
    unsigned long _lastMotorCommand = 0;
    bool _motorsActive = false;
    static const unsigned long MOTOR_SAFETY_TIMEOUT = 500; // ms

    // WiFi scan/connect state (Phase 3)
    bool _scanInProgress = false;
    String _fallbackSSID;
    String _fallbackPassword;
    String _initialSSID;
    String _initialPassword;

    // Internal handlers
    void handleWSMessage(const char* data, size_t len);
    void handleWSConnect(bool connected);
    void handleWSDisconnect(bool connected);
    void pushSystemInfo();
    void emergencyStop();

    // Internal broadcast helper (serializes protocol message and sends)
    void broadcast(char* buf, size_t len);
    void handleWifiScan();
    void handleWifiConnect(const String& ssid, const String& password);
    void saveWifiCredentials(const char* ssid, const char* password);
    bool loadWifiCredentials(String& ssid, String& password);
    void broadcastAlert(const char* msg, const char* alertType);
};
