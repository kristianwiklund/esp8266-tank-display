# ESP8266 Tank Level Meter for SignalK

A water tank level monitor running on an ESP8266 (NodeMCU v2). It reads a set of digital sensors positioned at different heights inside a tank, derives a fill level from their combined state, and publishes it to a SignalK server as a 0–1 ratio. An optional 0.96" OLED display shows the current level as a progress bar.

Based on https://github.com/peff74/esp8266_OLED_HW-364A

## How It Works

### Sensor Logic

The firmware expects one or more digital sensors (capacitive or ultrasonic) mounted at fixed heights in the tank, each wired to a separate GPIO pin with `INPUT_PULLUP`. Each sensor reads HIGH (1) when the fluid is present at that height and LOW (0) when it is not.

The pins are defined in an array (default: D1, D2, D4 from lowest to highest). On any state change, `calclevel()` scans the array from top to bottom and returns `i/numpins` as a float — i.e. the level is determined by whichever is the highest active sensor. For example, with three sensors:

| Sensors active | Level sent to SignalK |
|---|---|
| None | 0.00 (0%) |
| D1 only (lowest) | 0.33 (33%) |
| D1 + D2 | 0.67 (67%) |
| D1 + D2 + D4 (highest) | 1.00 (100%) |

The current level is broadcast to SignalK on every sensor change, and also re-broadcast on a configurable interval (default: every 10 seconds) even if nothing has changed.

### SignalK Integration

Level is published to the path `tanks.freshWater.0.currentLevel` as a number in the range 0–1. SensESP handles the WiFi connection, websocket communication, and device configuration portal.

### WiFi Configuration

WiFi credentials are not hardcoded in the firmware. Authentication is managed entirely by SensESP, which uses **ESPAsyncWiFiManager** internally:

1. **First boot** (or after credentials are erased): The device starts its own WiFi access point with a captive portal. Connect to it from a phone or laptop, then enter your WiFi SSID, password, and SignalK server address via the web form.
2. **Credentials are stored** to the ESP8266's flash memory by WifiManager.
3. **Subsequent boots**: The device reads credentials from flash and connects directly — no portal appears.

There is no place in the source code to hardcode WiFi credentials.

### OLED Display

The display shows:
- A header: "Freshwater Tank"
- A progress bar representing tank fill level
- A text label: "Level: XX%"

The display starts **off** by default (`blanked=true`). The button on **D3** toggles it on/off. Pressing D3 when the display is off turns it on; pressing again turns it off.

The OLED is driven by the U8g2 library with SSD1306 controller (128×64).

## Hardware

### Tested Board

**HW-364A / HW-364B** — an ESP8266 development board with a built-in 0.96" SSD1306 OLED. Sold widely under various names.

**Important:** On this board, SDA and SCL are swapped from the typical convention:
- SDA → GPIO14 (D6)
- SCL → GPIO12 (D5)
- I2C address: 0x3C

### Pin Assignment

| Pin | Function |
|-----|----------|
| D1  | Sensor 0 (lowest level) |
| D2  | Sensor 1 (mid level) |
| D4  | Sensor 2 (highest level) |
| D3  | Display toggle button |
| D5  | OLED SCL (GPIO12) |
| D6  | OLED SDA (GPIO14) |
| D0  | Boot / program (reserved) |

All sensor and button pins use `INPUT_PULLUP`. Sensors should pull the pin LOW when fluid is detected (active-low).

To add or remove sensors, edit the `pins[]` array in `src/water_tank_meter.ino`. The number of sensors is derived automatically from the array length.

## Dependencies

Built with **PlatformIO** targeting `espressif8266@4.1.0` / Arduino framework / NodeMCU v2.

| Library | Version / Source |
|---------|-----------------|
| SensESP | [kristianwiklund/SensESP@1.0.x_compile_fixes](https://github.com/kristianwiklund/SensESP/tree/1.0.x_compile_fixes) |
| ReactESP | 1.0.0 (via GitHub zip) |
| U8g2 | ^2.36.5 |
| Adafruit Unified Sensor | ^1.1.15 |
| ESP8266HTTPClient | bundled with platform |
| ArduinoOTA | bundled with platform |

### Note on SensESP fork

This project depends on a **fork** of SensESP rather than the published upstream release. The upstream `1.0.8-alpha` release does not compile against `espressif8266@4.1.0`. The fork at `kristianwiklund/SensESP`, branch `1.0.x_compile_fixes`, contains a single additional commit — "updated to compile with espressif8266 4.1.0" — that resolves these build failures. Do not switch back to the upstream `SensESP@^1.0.8-alpha` package without verifying it compiles.

Note also that SensESP v1.x is an older generation of the library targeting ESP8266 + Arduino. The current upstream SensESP (v3.x) targets ESP32 only and has a substantially different API. This project intentionally uses v1.x.

## Building and Flashing

```bash
# Build
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

Serial output is at 115200 baud and logs all sensor state changes, computed levels, and SensESP framework messages.

## Configuration

The following values can be changed at the top of `src/water_tank_meter.ino`:

| Variable | Default | Description |
|----------|---------|-------------|
| `broadcast_interval` | 10 | Seconds between periodic SignalK updates |
| `read_delay` | 10 | Debounce delay (ms) for digital inputs |
| `pins[]` | {D1, D2, D4} | Sensor input pins, ordered lowest to highest |
| `blanked` | true | Whether the display starts off |
