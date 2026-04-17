#pragma once
#include <functional>
#include <Arduino.h>  // for String

class ITransport {
public:
    virtual ~ITransport() = default;

    // Server lifecycle
    virtual bool begin(const char* ssid, const char* password, uint16_t port = 80) = 0;
    virtual void loop() = 0;

    // Static page serving (PROGMEM on Arduino)
    virtual void servePage(const uint8_t* page, size_t length, const char* mime = "text/html") = 0;

    // WebSocket callbacks
    using WSCallback = std::function<void(const char* data, size_t len)>;
    using ConnCallback = std::function<void(bool connected)>;

    virtual void onWSMessage(WSCallback cb) = 0;
    virtual void onWSConnect(ConnCallback cb) = 0;
    virtual void onWSDisconnect(ConnCallback cb) = 0;
    virtual void wsBroadcast(const char* data, size_t len) = 0;
    virtual bool wsConnected() const = 0;

    // System info (transport-agnostic accessors)
    virtual String getLocalIP() const = 0;
    virtual int32_t getRSSI() const = 0;
};
