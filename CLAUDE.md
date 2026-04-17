<!-- GSD:project-start source:PROJECT.md -->
## Project

**RobotWebUI**

A framework-agnostic C++ library that serves an eye-catching robot dashboard web page from an ESP8266. It provides live sensor readings, motor control, system status, and WiFi management — all over WebSocket. Designed as a drop-in template that robotics projects can integrate by instantiating a class and calling simple API methods, without touching the frontend code.

Built for ESP8266 (Arduino framework) first, with the C++ layer designed to compile under ESP-IDF later without rewriting.

**Core Value:** A reusable, modular robot web dashboard that any ESP8266/ESP32 project can integrate in minutes — live data, controls, WiFi config, all in one animated UI served from the microcontroller itself.

### Constraints

- **Memory**: ESP8266 has ~80KB total heap; HTML/CSS/JS must use PROGMEM and be streamed, not held in RAM
- **Framework**: Arduino framework for v1, but C++ class must not tightly couple to Arduino APIs (abstract server/WebSocket behind interface)
- **Board**: ESP-12E (ESP8266) — 4MB flash, 80MHz CPU
- **No external frontend build**: All HTML/CSS/JS is embedded in C++ source as PROGMEM strings — no npm, no bundler, no build step
- **Single-page**: One HTML page served at root, WebSocket connection for all data
- **Browser compatibility**: Must work on Chrome/Firefox/Safari on mobile and desktop
<!-- GSD:project-end -->

<!-- GSD:stack-start source:research/STACK.md -->
## Technology Stack

## Recommended Stack
### Core Framework and Platform
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| PlatformIO | Latest | Build system, dependency management | Project already uses it. Superior to Arduino IDE for library development: multi-environment builds, unit test runner, library dependency resolution via `library.json`. |
| ESP8266 Arduino Core | 3.1.2 | Hardware abstraction for ESP-12E | Latest stable release (March 2023). The ESP8266 platform is in maintenance mode -- no further major releases expected. This is the final stable version. Bundled with PlatformIO `espressif8266@4.2.1`. |
| Arduino framework | (via core) | Framework for v1 | Required for v1 delivery. The C++ library layer must abstract framework-specific APIs (HTTP server, WebSocket, WiFi) behind interfaces so the ESP-IDF port only requires new interface implementations, not a rewrite. |
### Web Server and Communication
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| **ESP32Async/ESPAsyncWebServer** | 3.10.3 | Async HTTP + WebSocket server | The actively maintained successor to `me-no-dev/ESPAsyncWebServer`. Moved to the ESP32Async GitHub organization in early 2026. Non-blocking async architecture is critical on ESP8266 where blocking HTTP would starve sensor loops and WebSocket handling. Built-in WebSocket support eliminates a separate library. Native PROGMEM serving via `request->send_P()`. Native ArduinoJson 7 integration. Supports ESP8266, ESP32, RP2040 out of the box. |
| **ESP32Async/ESPAsyncTCP** | 2.0.0 | Async TCP transport layer for ESP8266 | Required dependency of ESPAsyncWebServer on ESP8266. Maintained alongside the web server by the same ESP32Async org. The original `me-no-dev/ESPAsyncTCP` is abandoned (last push Jan 2025). |
### Data Serialization
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| **ArduinoJson** | 7.4.3 | JSON serialization/deserialization | The standard JSON library for embedded C++. v7 unified `JsonDocument` (no more `StaticJsonDocument` vs `DynamicJsonDocument` split). Very low memory footprint -- critical on ESP8266's ~50KB available heap. Used for structuring all WebSocket messages: sensor telemetry, motor commands, system info, WiFi scan results. ESPAsyncWebServer has native ArduinoJson 7 support. |
### Frontend (Embedded, No Build Tools)
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| Vanilla HTML/CSS/JS | (inline) | Dashboard UI | No npm, no bundler, no build step. All embedded as PROGMEM strings in C++ source. This is a hard constraint from the project requirements. Raw string literals (`R"rawliteral(...)rawliteral"`) keep HTML readable in C++ source. |
| CSS Custom Properties | (inline) | Design tokens, theming | Enables dark theme with consistent palette without a CSS preprocessor. The reference `.ino` uses `#0f172a` background, `#1e293b` cards, Tailwind-inspired colors. CSS custom properties replicate this pattern in embedded CSS. |
| Native WebSocket API | (browser) | Browser-side WebSocket client | Built into all modern browsers. No library needed. `new WebSocket('ws://' + location.host + '/ws')` works on Chrome, Firefox, Safari, mobile browsers. |
| `requestAnimationFrame` + CSS transitions | (inline) | Animations | For state-change animations (LED toggle, sensor value updates, badge transitions). No animation library -- the ESP8266 cannot serve large JS payloads. Keep JS under ~15KB raw (5-8KB gzipped). |
### Testing
| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| PlatformIO Native Test | Latest | Unit tests for C++ logic | PlatformIO's `[env:native]` target compiles C++ tests on the host machine. Test the library's JSON message building, state management, and callback logic without hardware. |
| Manual browser testing | - | WebSocket and UI validation | No practical way to automated-test embedded web UI on a microcontroller. Manual verification across Chrome, Firefox, Safari on mobile and desktop. |
## Alternatives Considered
| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| Web Server | ESP32Async/ESPAsyncWebServer | `me-no-dev/ESPAsyncWebServer` (original) | Abandoned. Last meaningful push Jan 2025. Over 200 open issues. Unmaintained code with known bugs. The ESP32Async fork is the legitimate successor maintained by the same community. |
| Web Server | ESP32Async/ESPAsyncWebServer | `ESP8266WebServer` (builtin sync server) | Synchronous and blocking. Each request ties up the CPU. WebSocket requires a separate library. Cannot handle concurrent connections. Would cause missed sensor readings during HTTP handling. |
| Web Server | ESP32Async/ESPAsyncWebServer | `Links2004/arduinoWebSockets` (standalone WebSocket) | Released v2.7.2 (Dec 2025), actively maintained, 2042 stars. However, it is a WebSocket-only library -- you still need a separate HTTP server for serving HTML. ESPAsyncWebServer bundles both HTTP + WebSocket in one dependency with a unified async architecture. Using two separate libraries adds complexity and potential conflicts. |
| JSON | ArduinoJson 7 | ArduinoJson 6 | v6 is legacy. v7 has a simpler API (single `JsonDocument` type), better memory efficiency, and active maintenance. v7.4.3 released March 2026. |
| JSON | ArduinoJson 7 | `cJSON` | C library, not C++-idiomatic. More manual memory management. ArduinoJson's RAII-based `JsonDocument` is safer and more convenient in C++. |
| Frontend | Vanilla HTML/CSS/JS | React/Vue/Svelte | Requires npm, bundler, build pipeline. Project constraint: no external frontend build tools. ESP8266 cannot serve the resulting bundle sizes anyway. |
| Frontend | Vanilla HTML/CSS/JS | LittleFS filesystem for HTML | Storing HTML/CSS/JS as files in LittleFS instead of PROGMEM. Simpler to edit but adds filesystem complexity and flash wear concerns. PROGMEM is more predictable and the project already uses it. Consider LittleFS only if HTML exceeds ~100KB raw. |
| Platform | `espressif8266@4.2.1` | ESP-IDF for ESP8266 | ESP-IDF support for ESP8266 is unofficial and poorly maintained. Arduino core is the standard path. The C++ abstraction layer handles the future ESP-IDF-on-ESP32 transition. |
## Installation
### platformio.ini
### Library Structure (for the RobotWebUI library itself)
## Confidence Assessment
| Recommendation | Confidence | Justification |
|----------------|------------|---------------|
| ESP32Async/ESPAsyncWebServer | **HIGH** | Verified via GitHub API: v3.10.3 released March 2026, actively pushed (April 2026), official successor to `me-no-dev` fork. `library.json` confirms ESP8266 support and `ESP32Async/ESPAsyncTCP` dependency. |
| ESP32Async/ESPAsyncTCP | **HIGH** | Required dependency explicitly declared in ESPAsyncWebServer's `library.json`. v2.0.0 released Jan 2025 by same organization. |
| ArduinoJson 7.4.3 | **HIGH** | Verified via GitHub API: v7.4.3 released March 2026, actively maintained by bblanchon. Undisputed standard for embedded JSON. |
| ESP8266 Arduino Core 3.1.2 | **HIGH** | Verified via GitHub API: latest release, March 2023. Platform is in maintenance mode. `espressif8266@4.2.1` PlatformIO platform bundles this core. |
| Vanilla HTML/CSS/JS (no framework) | **HIGH** | Hard project constraint. No realistic alternative given PROGMEM embedding and no build tools. |
| PlatformIO `espressif8266@4.2.1` | **HIGH** | Verified via GitHub API: latest PlatformIO platform release for ESP8266, July 2023. |
| `IWebServer` abstraction layer | **MEDIUM** | Design pattern is sound and widely used for framework portability. Cannot fully verify until implementation, but the ESPAsyncWebServer API is well-documented and straightforward to abstract. |
## Key Architecture Note: WebSocket Message Protocol
## Sources
- ESP32Async/ESPAsyncWebServer: https://github.com/ESP32Async/ESPAsyncWebServer (v3.10.3, 564 stars, pushed April 2026)
- ESP32Async/ESPAsyncTCP: https://github.com/ESP32Async/ESPAsyncTCP (v2.0.0)
- ArduinoJson: https://github.com/bblanchon/ArduinoJson (v7.4.3, released March 2026)
- ESP8266 Arduino Core: https://github.com/esp8266/Arduino (v3.1.2, released March 2023)
- PlatformIO espressif8266: https://github.com/platformio/platform-espressif8266 (v4.2.1)
- Original me-no-dev/ESPAsyncWebServer: https://github.com/me-no-dev/ESPAsyncWebServer (abandoned, last push Jan 2025)
- Links2004/arduinoWebSockets: https://github.com/Links2004/arduinoWebSockets (v2.7.2, considered but not chosen)
- ESP32Async documentation: https://ESP32Async.github.io/ESPAsyncWebServer/
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

- **Library structure**: Each library under `lib/` has its own `src/` with `.h`/`.cpp` pairs; no `library.json` (PlatformIO auto-discovers)
- **PROGMEM strings**: All HTML/CSS/JS in `pages.h` using `R"rawliteral(...)rawliteral"` raw string literals with `PROGMEM` attribute
- **WebSocket protocol**: JSON envelope `{t: "type", d: {...}}` — see `MsgType` namespace in `WSProtocol.h` for all message types
- **Typed callbacks**: Command structs (`MotorCmd`, `CoefCmd`, `WiFiCmd`) passed by const reference to user callbacks
- **Motor driver API**: `L293D::drive(direction, speed)` where direction is a string ("forward"/"back"/"left"/"right"/"stop") and speed is 0-100
- **Coefficients**: Stored as floats 0.0-1.0 in C++, sent as 0-100 percentage from UI
- **Minified CSS**: All CSS on single lines with shorthand properties (no preprocessor, no formatting)
- **Minified JS**: `var` declarations, short variable names, no ES6 modules (IE11-compatible where possible)
- **No comments in production code**: Frontend code in pages.h has minimal section comments only
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

```
src/main.cpp                 -- Application entry: WiFi credentials, wiring L293D + RobotWebUI
lib/
  L293D/                     -- L293D dual H-bridge motor driver library
    src/L293D.h              -- Config struct (leftDir, leftPWM, rightDir, rightPWM, maxPWM)
    src/L293D.cpp            -- PWM + direction control, per-motor coefficients, drive(direction, speed)
  RobotWebUI/                -- Web dashboard library (framework-agnostic core)
    src/ITransport.h         -- Abstract interface: HTTP serve, WebSocket, WiFi accessors
    src/ArduinoTransport.h/.cpp  -- ESPAsyncWebServer + ESPAsyncTCP implementation of ITransport
    src/WSProtocol.h/.cpp    -- JSON message serialization/deserialization (ArduinoJson 7)
    src/RobotWebUI.h/.cpp    -- Public API: begin(), onMotorCommand(), onCoefficientCommand(), push*()
    src/pages.h              -- Single-page HTML/CSS/JS dashboard (PROGMEM, ~7KB)
scripts/check_progspace.py   -- Post-build PROGMEM budget check (fails if INDEX_HTML > 40KB)
```

### Hardware Wiring (L293D on ESP-12E)
| Pin  | GPIO | L293D Function |
|------|------|----------------|
| D4   | 2    | Left motor direction |
| D2   | 4    | Left motor PWM (speed) |
| D3   | 0    | Right motor direction |
| D1   | 5    | Right motor PWM (speed) |

### Frontend (pages.h)
Single-page dashboard served from PROGMEM. Sections:
- **Connection status**: WebSocket state badge (connected/disconnected with dot indicator)
- **Motor controls**: Emergency stop button + D-pad (hold-to-drive with pointer events) + speed slider (0-100%)
- **Calibration**: Left/right motor coefficient sliders (0-100%, sent as 0.0-1.0 via WebSocket)
- **System info**: IP address, mDNS hostname, uptime (pushed every 3s from ESP)

WebSocket messages use `{t: "type", d: {...}}` envelope. Motor commands sent on pointer hold (200ms interval), coefficient changes sent on slider input. Auto-reconnect with exponential backoff (1s-10s). Motor safety timeout: 500ms server-side auto-stop.

### Data Flow
```
Browser --WS--> ArduinoTransport --> RobotWebUI.handleWSMessage()
                                        |
                            WSProtocol.parseMessageType()
                                |           |           |
                          MotorCmd      CoefCmd      WiFiCmd
                                |           |
                     onMotorCommand()  onCoefficientCommand()
                                |           |
                            L293D.drive()  L293D.setCoefficients()

RobotWebUI.pushSystemInfo() -- every 3s --> WSProtocol.buildSystemInfo() --> wsBroadcast --> Browser
```
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, or `.github/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
