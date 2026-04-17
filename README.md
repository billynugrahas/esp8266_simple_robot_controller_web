# Simple Robot Controller UI

A web-based robot controller for ESP8266. Serves a mobile-friendly dashboard with motor controls, calibration sliders, and live system info — all over WebSocket from the microcontroller itself.

No npm, no bundler, no build step. The entire frontend is embedded as a PROGMEM string in C++ source.

## Hardware

- **Board**: ESP-12E (ESP8266, 4MB flash, 80MHz)
- **Motor driver**: L293D dual H-bridge
- **Wiring**:

| ESP-12E Pin | GPIO | L293D |
|-------------|------|-------|
| D4 | 2 | Left motor direction |
| D2 | 4 | Left motor PWM |
| D3 | 0 | Right motor direction |
| D1 | 5 | Right motor PWM |

## Dashboard Features

- **D-pad motor control** — hold-to-drive with pointer events (works on touch and mouse)
- **Speed slider** — 0-100% throttle
- **Motor calibration** — left/right coefficient sliders for straight-line tracking
- **Emergency stop** — immediate motor cutoff
- **System info** — IP address, mDNS hostname, uptime (pushed every 3s)
- **Auto-reconnect** — exponential backoff (1s to 10s)
- **Motor safety timeout** — auto-stops motors after 500ms of no commands

## Project Structure

```
src/main.cpp                     Application entry point
lib/
  L293D/                         L293D H-bridge motor driver library
  RobotWebUI/                    Web dashboard library
    src/pages.h                  Embedded HTML/CSS/JS (PROGMEM, ~7KB)
scripts/check_progspace.py       Post-build PROGMEM budget check (40KB limit)
upload.sh                        Build, upload, and open serial monitor
```

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) CLI or VSCode extension
- ESP-12E connected via USB

### Build & Upload

```bash
# Build and upload to ESP-12E, then open serial monitor
./upload.sh
```

Or manually:

```bash
pio run -t upload -e esp12e
pio device monitor -b 115200
```

### Connect

1. The ESP8266 connects to WiFi using credentials in `src/main.cpp`
2. Open the serial monitor to see the assigned IP
3. Navigate to `http://<IP>/` in a browser
7. Alternatively, try `http://robot.local/` (mDNS)

### Customize WiFi

Edit the credentials in `src/main.cpp`:

```cpp
const char* WIFI_SSID = "YourNetwork";
const char* WIFI_PASS = "YourPassword";
```

## Build Environments

| Environment | Purpose |
|-------------|---------|
| `esp12e` | Full build with motor control, upload target |
| `esp12e-minimal` | Minimal build (alternative `minimal.cpp` entry) |
| `native` | Host-compiled unit tests (no hardware needed) |

```bash
pio run -e native           # Run native tests
pio run -e esp12e           # Build for ESP-12E
```

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| ESP32Async/ESPAsyncWebServer | ^3.10.3 | Async HTTP + WebSocket server |
| ESP32Async/ESPAsyncTCP | ^2.0.0 | Async TCP for ESP8266 |
| bblanchon/ArduinoJson | ^7.4.3 | JSON serialization |

## License

MIT
