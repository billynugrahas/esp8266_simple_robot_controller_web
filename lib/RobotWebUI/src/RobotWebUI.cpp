#include "RobotWebUI.h"
#include "ArduinoTransport.h"
#include "pages.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>

// EEPROM layout for WiFi credential persistence (D-08)
#define EEPROM_WIFI_SIZE   128
#define WIFI_MAGIC          0xA5
#define WIFI_ADDR_MAGIC     0
#define WIFI_ADDR_SSID      1
#define WIFI_ADDR_PASSWORD  33

RobotWebUI::RobotWebUI()
    : _transport(nullptr), _motorCount(2) {
}

void RobotWebUI::begin(const char* ssid, const char* password, int motorCount) {
    _motorCount = motorCount;

    _initialSSID = ssid;
    _initialPassword = password;

    // D-08: Try EEPROM credentials first for auto-connect on boot
    String savedSSID, savedPassword;
    if (loadWifiCredentials(savedSSID, savedPassword)) {
        Serial.printf("[RobotWebUI] Found saved WiFi: %s\n", savedSSID.c_str());
        ssid = savedSSID.c_str();
        password = savedPassword.c_str();
    }

    // Create Arduino transport implementation
    _transport = new ArduinoTransport();
    if (!_transport) {
        Serial.println("[RobotWebUI] FATAL: Failed to allocate transport");
        return;
    }

    // Register internal WebSocket callbacks
    _transport->onWSMessage([this](const char* data, size_t len) {
        handleWSMessage(data, len);
    });
    _transport->onWSConnect([this](bool connected) {
        handleWSConnect(connected);
    });
    _transport->onWSDisconnect([this](bool connected) {
        handleWSDisconnect(connected);
    });

    // Connect WiFi and start server
    if (!_transport->begin(ssid, password)) {
        Serial.println("[RobotWebUI] WiFi connection failed");
        return;
    }

    // Serve dashboard page from PROGMEM
    _transport->servePage((const uint8_t*)INDEX_HTML, sizeof(INDEX_HTML));

    Serial.printf("[RobotWebUI] Dashboard PROGMEM: %u bytes (%.1f KB)\n",
        sizeof(INDEX_HTML), sizeof(INDEX_HTML) / 1024.0);
    Serial.printf("[RobotWebUI] Estimated gzipped: ~%.1f KB (%.0f%% compression)\n",
        sizeof(INDEX_HTML) * 0.35 / 1024.0,
        65.0);
    Serial.printf("[RobotWebUI] Flash budget: %.1f KB / 45.0 KB (%.0f%% used)\n",
        sizeof(INDEX_HTML) / 1024.0,
        (sizeof(INDEX_HTML) / 1024.0) / 45.0 * 100.0);
    Serial.printf("[RobotWebUI] Started with %d motor(s)\n", _motorCount);
}

void RobotWebUI::loop() {
    if (!_transport) return;
    _transport->loop();

    // Motor safety timeout -- auto-stop after 500ms silence (MOTO-04 / D-03)
    if (_motorsActive && (millis() - _lastMotorCommand > MOTOR_SAFETY_TIMEOUT)) {
        emergencyStop();
    }

    // Periodic system info push (WIFI-02)
    if (millis() - _lastSystemPush >= SYSTEM_PUSH_INTERVAL) {
        pushSystemInfo();
    }
}

// D-02: typed push methods per sensor

void RobotWebUI::pushDistance(float cm) {
    if (!_transport || !_transport->wsConnected()) return;
    char buf[128];
    _protocol.buildDistance(cm, buf, sizeof(buf));
    broadcast(buf, strlen(buf));
}

void RobotWebUI::pushIR(bool detected) {
    if (!_transport || !_transport->wsConnected()) return;
    char buf[64];
    _protocol.buildIR(detected, buf, sizeof(buf));
    broadcast(buf, strlen(buf));
}

void RobotWebUI::pushOdometry(float x, float y, float heading) {
    if (!_transport || !_transport->wsConnected()) return;
    char buf[128];
    _protocol.buildOdometry(x, y, heading, buf, sizeof(buf));
    broadcast(buf, strlen(buf));
}

void RobotWebUI::pushBoolean(int index, bool state) {
    if (!_transport || !_transport->wsConnected()) return;
    char buf[64];
    _protocol.buildBoolean(index, state, buf, sizeof(buf));
    broadcast(buf, strlen(buf));
}

// D-03: per-type callbacks with typed structs

void RobotWebUI::onMotorCommand(void (*cb)(const MotorCmd&)) {
    _motorCallback = cb;
}

void RobotWebUI::onWiFiCommand(void (*cb)(const WiFiCmd&)) {
    _wifiCallback = cb;
}

void RobotWebUI::onCoefficientCommand(void (*cb)(const CoefCmd&)) {
    _coefCallback = cb;
}

// Internal handlers

void RobotWebUI::handleWSMessage(const char* data, size_t len) {
    const char* type = _protocol.parseMessageType(data, len);
    if (!type) return;  // Silently ignore invalid JSON (T-01-01)

    if (strcmp(type, MsgType::MOTOR) == 0) {
        if (_motorCallback) {
            MotorCmd cmd;
            JsonDocument& doc = _protocol.document();
            cmd.direction = doc["d"]["dir"] | "stop";
            cmd.speed = doc["d"]["spd"] | 0;
            // Validate direction -- silently ignore invalid commands
            if (cmd.direction != "forward" && cmd.direction != "back" &&
                cmd.direction != "left" && cmd.direction != "right" &&
                cmd.direction != "stop") {
                return;
            }
            _motorCallback(cmd);
            _lastMotorCommand = millis();
            _motorsActive = (cmd.direction != "stop");
        }
    } else if (strcmp(type, MsgType::WIFI_CMD) == 0) {
        JsonDocument& doc = _protocol.document();
        String action = doc["d"]["act"] | "";

        if (action == "scan") {
            handleWifiScan();
        } else if (action == "connect") {
            String ssid = doc["d"]["ssid"] | "";
            String pw = doc["d"]["pw"] | "";
            handleWifiConnect(ssid, pw);
        }
        // Still notify user callback for external handling
        if (_wifiCallback) {
            WiFiCmd cmd;
            cmd.action = action;
            cmd.ssid = doc["d"]["ssid"] | "";
            cmd.password = doc["d"]["pw"] | "";
            _wifiCallback(cmd);
        }
    } else if (strcmp(type, MsgType::COEF) == 0) {
        if (_coefCallback) {
            CoefCmd cmd;
            JsonDocument& doc = _protocol.document();
            cmd.left = doc["d"]["left"] | 1.0f;
            cmd.right = doc["d"]["right"] | 1.0f;
            _coefCallback(cmd);
        }
    }
    // Unknown types ignored silently
}

void RobotWebUI::handleWSConnect(bool connected) {
    Serial.println("[RobotWebUI] WebSocket client connected");
    _motorsActive = false;
}

void RobotWebUI::handleWSDisconnect(bool connected) {
    Serial.println("[RobotWebUI] WebSocket client disconnected");
    if (_motorsActive) {
        emergencyStop();
    }
}

void RobotWebUI::pushSystemInfo() {
    if (!_transport) return;

    String ip = _transport->getLocalIP();
    uint32_t uptime = millis() / 1000;
    uint32_t heapKB = ESP.getFreeHeap() / 1024;
    int32_t rssi = _transport->getRSSI();

    char buf[192];
    _protocol.buildSystemInfo(ip.c_str(), uptime, heapKB, rssi, buf, sizeof(buf));
    broadcast(buf, strlen(buf));

    _lastSystemPush = millis();
}

void RobotWebUI::broadcast(char* buf, size_t len) {
    if (_transport && _transport->wsConnected()) {
        _transport->wsBroadcast(buf, len);
    }
}

void RobotWebUI::emergencyStop() {
    if (_motorCallback) {
        MotorCmd stopCmd;
        stopCmd.direction = "stop";
        stopCmd.speed = 0;
        _motorCallback(stopCmd);
    }
    _motorsActive = false;
    Serial.println("[RobotWebUI] Motor safety timeout -- all motors stopped");
    // Notify browser of motor timeout alert (D-12)
    if (_transport && _transport->wsConnected()) {
        char buf[64];
        _protocol.buildAck("motor_timeout", buf, sizeof(buf));
        _transport->wsBroadcast(buf, strlen(buf));
    }
}

void RobotWebUI::handleWifiScan() {
    if (_scanInProgress) return;  // Guard against overlapping scans (Pitfall 4)
    _scanInProgress = true;

    WiFi.scanNetworksAsync([this](int networksFound) {
        _scanInProgress = false;
        char buf[768];
        int len = _protocol.buildWifiScan(buf, sizeof(buf), networksFound);
        if (len > 0) broadcast(buf, len);
        WiFi.scanDelete();  // Free scan result memory (Pitfall 1)
    });
}

void RobotWebUI::handleWifiConnect(const String& ssid, const String& password) {
    if (ssid.length() == 0) return;  // Validate non-empty SSID (T-03-02)

    // Save current credentials for fallback on failure
    _fallbackSSID = WiFi.SSID();
    _fallbackPassword = WiFi.psk();

    WiFi.disconnect(true);

    // Handle open networks (no password) per Pitfall 5
    if (password.length() == 0) {
        WiFi.begin(ssid.c_str());
    } else {
        WiFi.begin(ssid.c_str(), password.c_str());
    }

    // Wait for connection with 15-second timeout
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
        // D-08: Save to EEPROM for persistence across reboots
        saveWifiCredentials(ssid.c_str(), password.c_str());

        // D-05: Send success alert with new IP BEFORE the socket drops
        char msg[128];
        snprintf(msg, sizeof(msg), "Connected to %s -- IP: %s",
                 ssid.c_str(), WiFi.localIP().toString().c_str());
        broadcastAlert(msg, "success");
        delay(200);  // Ensure alert reaches browser before potential disconnect
    } else {
        // D-05: Fall back to original network
        WiFi.disconnect();
        if (_fallbackSSID.length() > 0) {
            WiFi.begin(_fallbackSSID.c_str(), _fallbackPassword.c_str());
            unsigned long fbStart = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - fbStart < 10000) {
                delay(100);
            }
        }
        broadcastAlert("Connection failed -- check password", "danger");
    }
}

void RobotWebUI::saveWifiCredentials(const char* ssid, const char* password) {
    EEPROM.begin(EEPROM_WIFI_SIZE);

    // Check if credentials already match -- avoid unnecessary flash writes (Pitfall 3)
    if (EEPROM.read(WIFI_ADDR_MAGIC) == WIFI_MAGIC) {
        bool match = true;
        for (int i = 0; i < 32; i++) {
            char expected = (i < (int)strlen(ssid)) ? ssid[i] : 0;
            if (EEPROM.read(WIFI_ADDR_SSID + i) != expected) { match = false; break; }
        }
        if (match) {
            for (int i = 0; i < 64; i++) {
                char expected = (i < (int)strlen(password)) ? password[i] : 0;
                if (EEPROM.read(WIFI_ADDR_PASSWORD + i) != expected) { match = false; break; }
            }
        }
        if (match) { EEPROM.end(); return; }  // No write needed
    }

    EEPROM.write(WIFI_ADDR_MAGIC, WIFI_MAGIC);
    // Write SSID (32 bytes, null-padded)
    for (int i = 0; i < 32; i++) {
        EEPROM.write(WIFI_ADDR_SSID + i, (i < (int)strlen(ssid)) ? ssid[i] : 0);
    }
    // Write password (64 bytes, null-padded)
    for (int i = 0; i < 64; i++) {
        EEPROM.write(WIFI_ADDR_PASSWORD + i, (i < (int)strlen(password)) ? password[i] : 0);
    }
    EEPROM.commit();
    EEPROM.end();
    Serial.printf("[RobotWebUI] WiFi credentials saved to EEPROM\n");
}

bool RobotWebUI::loadWifiCredentials(String& ssid, String& password) {
    EEPROM.begin(EEPROM_WIFI_SIZE);
    if (EEPROM.read(WIFI_ADDR_MAGIC) != WIFI_MAGIC) {
        EEPROM.end();
        return false;
    }
    char bufSSID[33] = {0};
    char bufPw[65] = {0};
    for (int i = 0; i < 32; i++) bufSSID[i] = EEPROM.read(WIFI_ADDR_SSID + i);
    bufSSID[32] = '\0';
    for (int i = 0; i < 64; i++) bufPw[i] = EEPROM.read(WIFI_ADDR_PASSWORD + i);
    bufPw[64] = '\0';
    EEPROM.end();

    if (bufSSID[0] == '\0') return false;
    ssid = String(bufSSID);
    password = String(bufPw);
    return true;
}

void RobotWebUI::broadcastAlert(const char* msg, const char* alertType) {
    char buf[192];
    _protocol.buildAlert(msg, alertType, buf, sizeof(buf));
    broadcast(buf, strlen(buf));
}
