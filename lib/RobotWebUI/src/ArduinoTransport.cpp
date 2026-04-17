#include "ArduinoTransport.h"

ArduinoTransport::ArduinoTransport()
    : _server(80), _ws("/ws") {
}

ArduinoTransport::~ArduinoTransport() {
    _ws.enable(false);
    _server.end();
}

bool ArduinoTransport::begin(const char* ssid, const char* password, uint16_t port) {
    Serial.printf("[RobotWebUI] Connecting to WiFi SSID: %s", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // Wait for connection with 20-second timeout
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttempt > 20000) {
            Serial.println("\n[RobotWebUI] WiFi connection timeout!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.printf("\n[RobotWebUI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

    // Start mDNS responder — access via http://robot.local
    if (MDNS.begin("robot")) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("[RobotWebUI] mDNS started: http://robot.local");
    } else {
        Serial.println("[RobotWebUI] mDNS failed to start");
    }

    // Register WebSocket event handler
    _ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                        AwsEventType type, void *arg, uint8_t *data, size_t len) {
        onWSEvent(server, client, type, arg, data, len);
    });

    // Add WebSocket handler to server
    _server.addHandler(&_ws);
    _server.begin();

    Serial.println("[RobotWebUI] Web server started.");
    return true;
}

void ArduinoTransport::loop() {
    // Required on ESP8266 to process mDNS queries
    MDNS.update();

    // Periodic client cleanup to prevent stale client accumulation (Pitfall 2)
    if (millis() - _lastCleanup >= CLEANUP_INTERVAL) {
        _ws.cleanupClients();
        _lastCleanup = millis();
    }
}

void ArduinoTransport::servePage(const uint8_t* page, size_t length, const char* mime) {
    _server.on("/", HTTP_GET, [page, length, mime](AsyncWebServerRequest *request) {
        // beginResponse_P streams from PROGMEM without RAM copy (CORE-04)
        auto* response = request->beginResponse_P(200, mime, page, length);
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    });
}

void ArduinoTransport::onWSMessage(WSCallback cb) {
    _onMessage = cb;
}

void ArduinoTransport::onWSConnect(ConnCallback cb) {
    _onConnect = cb;
}

void ArduinoTransport::onWSDisconnect(ConnCallback cb) {
    _onDisconnect = cb;
}

void ArduinoTransport::wsBroadcast(const char* data, size_t len) {
    _ws.textAll(data, len);
}

bool ArduinoTransport::wsConnected() const {
    return _ws.count() > 0;
}

String ArduinoTransport::getLocalIP() const {
    return WiFi.localIP().toString();
}

int32_t ArduinoTransport::getRSSI() const {
    return WiFi.RSSI();
}

void ArduinoTransport::onWSEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[RobotWebUI] WS client #%u connected\n", client->id());
            if (_onConnect) _onConnect(true);
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("[RobotWebUI] WS client #%u disconnected\n", client->id());
            if (_onDisconnect) _onDisconnect(false);
            break;
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len) {
                // Single-frame message -- process directly
                if (_onMessage && data && len > 0) {
                    Serial.printf("[WS] recv %d bytes: %.*s\n", (int)len, (int)len, (const char*)data);
                    _onMessage((const char*)data, len);
                }
            } else {
                Serial.printf("[WS] multi-frame: final=%d index=%u len=%u chunk=%u\n",
                    info->final, (unsigned)info->index, (unsigned)info->len, (unsigned)len);
            }
            break;
        }
        default:
            break;
    }
}
