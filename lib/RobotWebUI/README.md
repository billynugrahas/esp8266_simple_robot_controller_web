# RobotWebUI

A drop-in animated robot dashboard web page for ESP8266/ESP32 -- live sensors, motor control, WiFi management, all over WebSocket.

**Version:** 0.1.0

## Quick Start

```cpp
#include <RobotWebUI.h>

RobotWebUI ui;

void setup() {
    Serial.begin(115200);
    ui.begin("YourSSID", "YourPassword");
}

void loop() {
    ui.loop();
}
```

Upload the sketch, open Serial Monitor at 115200 baud, and navigate to the IP address printed in the Serial output. The dashboard loads in your browser with live system info updating every 3 seconds.

No external frontend build tools required. The entire dashboard is served from PROGMEM on the ESP8266 itself.

## Installation

### PlatformIO (recommended)

Add to `platformio.ini`:

```ini
lib_deps =
    RobotWebUI
```

Or clone directly into `lib/`:

```bash
git clone https://github.com/your-org/RobotWebUI.git lib/RobotWebUI
```

PlatformIO resolves these dependencies automatically from `library.json`:

| Dependency | Version | Purpose |
|------------|---------|---------|
| ESP32Async/ESPAsyncWebServer | ^3.10.3 | Async HTTP + WebSocket server |
| ESP32Async/ESPAsyncTCP | ^2.0.0 | Async TCP for ESP8266 |
| bblanchon/ArduinoJson | ^7.4.3 | JSON serialization |

### Arduino IDE

Install the three libraries above manually via Library Manager or ZIP download, then copy the `RobotWebUI` folder to your `libraries/` directory.

## API Reference

### RobotWebUI()

```cpp
RobotWebUI ui;
```

Constructor. Takes no arguments. Instantiate as a global object so it persists across `setup()` and `loop()`.

**Parameters:** None

**Return:** `RobotWebUI` object

---

### void begin(const char\* ssid, const char\* password, int motorCount = 2)

```cpp
ui.begin("MyNetwork", "MyPassword");
ui.begin("MyNetwork", "MyPassword", 4); // 4-motor (mecanum) setup
```

Connects to WiFi, starts the async web server, and serves the dashboard page. Call once in `setup()`. If EEPROM contains saved WiFi credentials from a prior connection, those are used instead.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| ssid | `const char*` | required | WiFi network name |
| password | `const char*` | required | WiFi password (use `""` for open networks) |
| motorCount | `int` | 2 | Number of motors. 2 = differential drive, 4 = mecanum |

**Return:** `void`

**Note:** Prints the dashboard IP address and PROGMEM size to Serial on boot.

---

### void loop()

```cpp
void loop() {
    ui.loop();
}
```

Must be called in every `loop()` iteration. Handles WebSocket messages, motor safety timeout, and periodic system info push. Omitting this call breaks all dashboard functionality.

**Parameters:** None

**Return:** `void`

---

### void pushDistance(float cm)

```cpp
// In loop(), read your ultrasonic sensor:
float distance = readUltrasonicCM();
ui.pushDistance(distance);
```

Pushes an ultrasonic distance reading to the dashboard. The browser displays a color-coded value: green above 30cm, yellow between 15-30cm, red below 15cm.

| Parameter | Type | Description |
|-----------|------|-------------|
| cm | `float` | Distance in centimeters |

**Return:** `void`

**Only sends if a WebSocket client is connected.** Safe to call at any rate; consider throttling to 100-500ms intervals.

---

### void pushIR(bool detected)

```cpp
bool obstacle = digitalRead(IR_PIN) == LOW;
ui.pushIR(obstacle);
```

Pushes IR proximity sensor state. The dashboard shows "Detected" in red or "Clear" in green.

| Parameter | Type | Description |
|-----------|------|-------------|
| detected | `bool` | `true` if obstacle detected, `false` if clear |

**Return:** `void`

---

### void pushOdometry(float x, float y, float heading)

```cpp
ui.pushOdometry(robotX, robotY, robotHeading);
```

Pushes odometry (position and heading) data. The dashboard displays X, Y position in your units and heading in degrees.

| Parameter | Type | Description |
|-----------|------|-------------|
| x | `float` | X position |
| y | `float` | Y position |
| heading | `float` | Heading angle in degrees |

**Return:** `void`

---

### void pushBoolean(int index, bool state)

```cpp
ui.pushBoolean(0, digitalRead(LIMIT_SWITCH_PIN)); // Limit switch 0
ui.pushBoolean(1, digitalRead(BUMPER_PIN));        // Bumper sensor 1
```

Pushes a boolean state (limit switch, bumper, etc.) to the dashboard. Displays as indexed "ON"/"OFF" badges.

| Parameter | Type | Description |
|-----------|------|-------------|
| index | `int` | 0-based sensor index |
| state | `bool` | `true` = ON, `false` = OFF |

**Return:** `void`

---

### void onMotorCommand(void (\*cb)(const MotorCmd&))

```cpp
void handleMotor(const MotorCmd& cmd) {
    Serial.printf("Motor: dir=%s speed=%d%%\n",
        cmd.direction.c_str(), cmd.speed);
    // Drive motors using cmd.direction and cmd.speed
}

void setup() {
    ui.onMotorCommand(handleMotor);
    ui.begin("SSID", "password");
}
```

Registers a callback invoked when the user presses D-pad buttons or triggers an emergency stop. Must be called **before** `begin()` to receive the initial connection cleanup.

| Parameter | Type | Description |
|-----------|------|-------------|
| cb | `void (*)(const MotorCmd&)` | Callback function receiving motor command |

**Return:** `void`

**Safety:** If no motor command is received within 500ms, the library auto-stops all motors and sends a `motor_timeout` ack to the browser. Release of D-pad buttons triggers a stop command.

---

### void onWiFiCommand(void (\*cb)(const WiFiCmd&))

```cpp
void handleWiFi(const WiFiCmd& cmd) {
    if (cmd.action == "scan") {
        Serial.println("WiFi scan requested from dashboard");
    } else if (cmd.action == "connect") {
        Serial.printf("Connect to: %s\n", cmd.ssid.c_str());
    }
}

void setup() {
    ui.onWiFiCommand(handleWiFi);
    ui.begin("SSID", "password");
}
```

Registers a callback invoked when the user triggers a WiFi scan or connect action from the dashboard. The library handles the actual scan/connect internally; this callback is for external notification.

| Parameter | Type | Description |
|-----------|------|-------------|
| cb | `void (*)(const WiFiCmd&)` | Callback function receiving WiFi command |

**Return:** `void`

---

## Callback Structs

### MotorCmd

| Field | Type | Description | Valid Values |
|-------|------|-------------|--------------|
| direction | `String` | Motor direction | `"forward"`, `"back"`, `"left"`, `"right"`, `"stop"` |
| speed | `int` | Motor speed percentage | `0` - `100` |

Invalid direction values are silently ignored by the library. The `speed` field is always present but may be `0` for stop commands.

### WiFiCmd

| Field | Type | Description | Valid Values |
|-------|------|-------------|--------------|
| action | `String` | WiFi action type | `"scan"`, `"connect"` |
| ssid | `String` | Network name | Non-empty string for connect, empty for scan |
| password | `String` | Network password | May be empty for open networks |

## WebSocket JSON Protocol

All messages use the envelope format: `{"t":"type","d":{payload}}`

The `t` field is the message type string. The `d` field contains the payload object.

### ESP8266 to Browser

| Type | Description | Example |
|------|-------------|---------|
| `sys` | System info (IP, uptime, heap, RSSI) | `{"t":"sys","d":{"ip":"192.168.1.100","up":42,"heap":45,"rssi":-60}}` |
| `dist` | Distance sensor reading | `{"t":"dist","d":{"cm":25.5}}` |
| `ir` | IR proximity state | `{"t":"ir","d":{"det":true}}` |
| `odo` | Odometry position | `{"t":"odo","d":{"x":1.20,"y":3.40,"hdg":90.0}}` |
| `bool` | Boolean/switch state | `{"t":"bool","d":{"idx":0,"st":true}}` |
| `wifi_scan` | WiFi scan results | `{"t":"wifi_scan","d":{"networks":[{"ssid":"MyNet","rssi":-55,"enc":3}]}}` |
| `ack` | Acknowledgment or alert | `{"t":"ack","d":{"msg":"motor_timeout"}}` or `{"t":"ack","d":{"msg":"Connected","alert":"success"}}` |

### Browser to ESP8266

| Type | Description | Example |
|------|-------------|---------|
| `motor` | Motor command from D-pad | `{"t":"motor","d":{"dir":"forward","spd":75}}` |
| `wifi` | WiFi scan or connect request | `{"t":"wifi","d":{"act":"scan"}}` or `{"t":"wifi","d":{"act":"connect","ssid":"MyNet","pw":"pass123"}}` |

### JSON Field Reference

| Payload | Field | Type | Description |
|---------|-------|------|-------------|
| sys | ip | string | Device IP address |
| sys | up | number | Uptime in seconds |
| sys | heap | number | Free heap in KB |
| sys | rssi | number | WiFi signal strength in dBm |
| dist | cm | number | Distance in centimeters |
| ir | det | boolean | `true` if obstacle detected |
| odo | x | number | X position |
| odo | y | number | Y position |
| odo | hdg | number | Heading in degrees |
| bool | idx | number | 0-based sensor index |
| bool | st | boolean | `true` = ON, `false` = OFF |
| motor | dir | string | Direction: "forward", "back", "left", "right", "stop" |
| motor | spd | number | Speed: 0-100 |
| wifi | act | string | Action: "scan" or "connect" |
| wifi | ssid | string | Network name (connect only) |
| wifi | pw | string | Network password (connect only) |
| wifi_scan | networks | array | Array of `{ssid, rssi, enc}` objects |
| ack | msg | string | Acknowledgment message |
| ack | alert | string | Alert type: "success", "danger" (optional) |

## PROGMEM Budget

The dashboard HTML/CSS/JS is stored in flash via PROGMEM and served directly. No RAM is held for the page content.

| Metric | Value |
|--------|-------|
| Current HTML/CSS/JS size | ~16.1 KB raw |
| Estimated gzipped | ~5.5 KB |
| Flash budget target | 45 KB |
| Flash warning threshold | 40 KB |
| Current headroom | ~64% |

The library logs the exact PROGMEM size to Serial on every boot:

```
[RobotWebUI] Dashboard PROGMEM: 16463 bytes (16.1 KB)
[RobotWebUI] Estimated gzipped: ~5.7 KB (65% compression)
[RobotWebUI] Flash budget: 16.1 KB / 45.0 KB (36% used)
```

## Browser Compatibility

| Browser | Minimum Version |
|---------|----------------|
| Chrome | 80+ |
| Firefox | 75+ |
| Safari | 13+ |
| Chrome Mobile | 80+ |
| Safari Mobile (iOS) | 13+ |

The dashboard uses native WebSocket API, CSS Custom Properties, CSS transitions, and ES5-compatible JavaScript. No polyfills or transpilation required.

## Advanced: Custom Transport

The library uses an `ITransport` interface internally to abstract the HTTP server and WebSocket layer. The default implementation (`ArduinoTransport`) uses ESPAsyncWebServer for the Arduino framework.

For ESP-IDF or other frameworks, implement the `ITransport` interface defined in `ITransport.h`:

```cpp
class ITransport {
public:
    virtual ~ITransport() = default;
    virtual bool begin(const char* ssid, const char* password, uint16_t port = 80) = 0;
    virtual void loop() = 0;
    virtual void servePage(const uint8_t* page, size_t length, const char* mime = "text/html") = 0;
    virtual void onWSMessage(WSCallback cb) = 0;
    virtual void onWSConnect(ConnCallback cb) = 0;
    virtual void onWSDisconnect(ConnCallback cb) = 0;
    virtual void wsBroadcast(const char* data, size_t len) = 0;
    virtual bool wsConnected() const = 0;
    virtual String getLocalIP() const = 0;
    virtual int32_t getRSSI() const = 0;
};
```

ESP-IDF support is planned for a future release. The interface exists today so the architecture is ready.

## Heap Stability Verification

Follow this procedure to verify memory stability during extended operation:

1. Upload the full demo sketch (`src/main.cpp`) to your ESP8266
2. Open Serial Monitor at **115200 baud**
3. Connect to the dashboard from a browser
4. Let the sketch run for **30+ minutes** with active sensor updates
5. Observe the `[Demo] Free heap: XXXXX bytes` output printed every 5 seconds
6. Verify the free heap stays within **+/-1 KB** of the initial value
7. If heap trends consistently downward over the observation period, there is a memory leak -- report it as a bug

The library uses a pre-allocated `JsonDocument` for WebSocket serialization to avoid per-message heap allocation. The only dynamic allocation is the `ArduinoTransport` object created once in `begin()`.

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| Dashboard does not load | WiFi not connected | Check Serial output for connection status. Verify SSID and password in `begin()`. |
| WebSocket keeps reconnecting | Unstable WiFi or out of range | Check the RSSI value on the dashboard. Move closer to the router. Values below -80 dBm indicate poor signal. |
| Motors do not respond | No callback registered | Call `ui.onMotorCommand()` before `ui.begin()` to ensure the callback is set. |
| Motor auto-stops after 500ms | Safety timeout (by design) | D-pad commands must be held continuously. Releasing the button triggers a stop command. The 500ms timeout is a safety feature. |
| WiFi scan returns empty results | Previous scan still in progress | Wait for the current scan to complete before scanning again. The library guards against overlapping scans. |
| EEPROM credentials not loading | First boot or corrupted data | The library falls back to the `begin()` SSID/password if EEPROM contains no valid credentials (magic byte check). |
| Dashboard layout appears broken | Old browser without CSS Custom Properties | Requires Chrome 80+, Firefox 75+, or Safari 13+. |
| Dashboard loads but data does not update | WebSocket connection failed | Check browser developer console for WebSocket errors. The dashboard auto-reconnects with exponential backoff (1s, 2s, 4s, 8s, max 10s). |
| Heap memory decreasing over time | Possible memory leak | Run the heap stability verification procedure above. If confirmed, report with the sketch code and observation duration. |
