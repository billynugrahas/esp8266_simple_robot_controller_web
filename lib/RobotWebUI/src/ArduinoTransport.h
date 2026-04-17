#pragma once
#include "ITransport.h"
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

class ArduinoTransport : public ITransport {
public:
    ArduinoTransport();
    ~ArduinoTransport() override;

    bool begin(const char* ssid, const char* password, uint16_t port = 80) override;
    void loop() override;
    void servePage(const uint8_t* page, size_t length, const char* mime = "text/html") override;

    void onWSMessage(WSCallback cb) override;
    void onWSConnect(ConnCallback cb) override;
    void onWSDisconnect(ConnCallback cb) override;
    void wsBroadcast(const char* data, size_t len) override;
    bool wsConnected() const override;

    String getLocalIP() const override;
    int32_t getRSSI() const override;

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    WSCallback _onMessage = nullptr;
    ConnCallback _onConnect = nullptr;
    ConnCallback _onDisconnect = nullptr;

    unsigned long _lastCleanup = 0;
    static const unsigned long CLEANUP_INTERVAL = 30000; // 30s client cleanup

    void onWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len);
};
