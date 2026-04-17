#pragma once
#include <ArduinoJson.h>

// Message type string constants (used as "t" field in JSON)
namespace MsgType {
    constexpr const char* SYSTEM   = "sys";
    constexpr const char* DISTANCE = "dist";
    constexpr const char* IR       = "ir";
    constexpr const char* ODOMETRY = "odo";
    constexpr const char* BOOLEAN  = "bool";
    constexpr const char* MOTOR    = "motor";
    constexpr const char* WIFI_CMD  = "wifi";
    constexpr const char* WIFI_SCAN = "wifi_scan";
    constexpr const char* ACK       = "ack";
    constexpr const char* COEF      = "coef";
}

// Typed command structs for callbacks (D-03)
struct MotorCmd {
    String direction;  // "forward", "back", "left", "right", "stop"
    int speed;         // 0-100
};

struct WiFiCmd {
    String action;     // "scan", "connect"
    String ssid;
    String password;
};

struct CoefCmd {
    float left;        // 0.0 - 1.0
    float right;       // 0.0 - 1.0
};

// WSProtocol class -- manages a pre-allocated JsonDocument for zero per-message heap (CORE-03)
class WSProtocol {
public:
    WSProtocol();
    void buildSystemInfo(const char* ip, uint32_t uptime, uint32_t heapKB, int32_t rssi, char* buf, size_t bufSize);
    void buildDistance(float cm, char* buf, size_t bufSize);
    void buildIR(bool detected, char* buf, size_t bufSize);
    void buildOdometry(float x, float y, float heading, char* buf, size_t bufSize);
    void buildBoolean(int index, bool state, char* buf, size_t bufSize);
    void buildAck(const char* msg, char* buf, size_t bufSize);
    int buildWifiScan(char* buf, size_t bufSize, int networkCount);
    void buildAlert(const char* msg, const char* alertType, char* buf, size_t bufSize);

    // Parse incoming message type. Returns nullptr for unknown types.
    const char* parseMessageType(const char* data, size_t len);

    // Pre-allocated document for serialization
    JsonDocument& document();

private:
    JsonDocument _doc;
};
