#include "WSProtocol.h"
#include <ESP8266WiFi.h>

WSProtocol::WSProtocol() {
}

void WSProtocol::buildSystemInfo(const char* ip, uint32_t uptime, uint32_t heapKB, int32_t rssi,
                                  char* buf, size_t bufSize) {
    _doc.clear();
    _doc["t"] = MsgType::SYSTEM;
    JsonObject d = _doc["d"].to<JsonObject>();
    d["ip"] = ip;
    d["up"] = uptime;
    d["heap"] = heapKB;
    d["rssi"] = rssi;
    serializeJson(_doc, buf, bufSize);
}

void WSProtocol::buildDistance(float cm, char* buf, size_t bufSize) {
    _doc.clear();
    _doc["t"] = MsgType::DISTANCE;
    JsonObject d = _doc["d"].to<JsonObject>();
    d["cm"] = cm;
    serializeJson(_doc, buf, bufSize);
}

void WSProtocol::buildIR(bool detected, char* buf, size_t bufSize) {
    _doc.clear();
    _doc["t"] = MsgType::IR;
    JsonObject d = _doc["d"].to<JsonObject>();
    d["det"] = detected;
    serializeJson(_doc, buf, bufSize);
}

void WSProtocol::buildOdometry(float x, float y, float heading, char* buf, size_t bufSize) {
    _doc.clear();
    _doc["t"] = MsgType::ODOMETRY;
    JsonObject d = _doc["d"].to<JsonObject>();
    d["x"] = x;
    d["y"] = y;
    d["hdg"] = heading;
    serializeJson(_doc, buf, bufSize);
}

void WSProtocol::buildBoolean(int index, bool state, char* buf, size_t bufSize) {
    _doc.clear();
    _doc["t"] = MsgType::BOOLEAN;
    JsonObject d = _doc["d"].to<JsonObject>();
    d["idx"] = index;
    d["st"] = state;
    serializeJson(_doc, buf, bufSize);
}

void WSProtocol::buildAck(const char* msg, char* buf, size_t bufSize) {
    _doc.clear();
    _doc["t"] = MsgType::ACK;
    JsonObject d = _doc["d"].to<JsonObject>();
    d["msg"] = msg;
    serializeJson(_doc, buf, bufSize);
}

int WSProtocol::buildWifiScan(char* buf, size_t bufSize, int networkCount) {
    _doc.clear();
    _doc["t"] = MsgType::WIFI_SCAN;
    JsonObject d = _doc["d"].to<JsonObject>();
    JsonArray networks = d["networks"].to<JsonArray>();

    for (int i = 0; i < networkCount && i < 15; i++) {
        JsonObject net = networks.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["enc"] = WiFi.encryptionType(i);
    }

    return serializeJson(_doc, buf, bufSize);
}

void WSProtocol::buildAlert(const char* msg, const char* alertType, char* buf, size_t bufSize) {
    _doc.clear();
    _doc["t"] = MsgType::ACK;
    JsonObject d = _doc["d"].to<JsonObject>();
    d["msg"] = msg;
    d["alert"] = alertType;
    serializeJson(_doc, buf, bufSize);
}

const char* WSProtocol::parseMessageType(const char* data, size_t len) {
    _doc.clear();
    DeserializationError error = deserializeJson(_doc, data, len);
    if (error) {
        return nullptr;
    }
    return _doc["t"] | nullptr;
}

JsonDocument& WSProtocol::document() {
    return _doc;
}
