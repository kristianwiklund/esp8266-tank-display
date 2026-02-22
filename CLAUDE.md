# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP8266-based water tank level meter for SignalK integration. Uses multiple capacitive/ultrasonic sensors on digital input pins to detect fluid height thresholds, combines readings into a 0–1 ratio, and publishes to SignalK under `tanks.freshWater.0.currentLevel`. An optional 0.96" SSD1306 OLED display (U8g2 library) shows a progress bar of tank level.

Target hardware: NodeMCU v2 (ESP8266), HW-364A/HW-364B board with built-in OLED display.

## Build System

This project uses **PlatformIO** (not Arduino IDE). All build/upload/monitor commands use the `pio` CLI.

```bash
# Build
pio run

# Upload to device
pio run --target upload

# Monitor serial output (115200 baud)
pio device monitor

# Build + upload + monitor in one step
pio run --target upload && pio device monitor
```

The PlatformIO environment is `nodemcuv2` (`platformio.ini`).

## Key Configuration (src/water_tank_meter.ino)

- `broadcast_interval` — seconds between SignalK transmits (default: 10)
- `pins[]` — digital input pins for sensors (default: D1, D2, D4)
- **Reserved pins**: D3 = display toggle button; D5/D6 = OLED SCL/SDA; D0 = boot/program

## Architecture

All logic lives in a single file: `src/water_tank_meter.ino`.

**SensESP framework** drives the application via `ReactESP`. The entry point is the `ReactESP app([]() { ... })` lambda, which runs once at startup and registers all reactive event handlers.

**Sensor reading logic** (`calclevel`): Scans `inputs[]` array from highest sensor to lowest. Returns `i/numpins` as a float ratio — the highest active sensor determines the level.

**`FloatSensor` class**: Extends `NumericSensor` to wrap a float value. Emits immediately on `set()` and re-emits on a repeating interval via `app.onRepeat()` for periodic SignalK broadcasts.

**Display blanking**: `blanked` global toggles OLED on/off. The D3 button is watched via `DigitalInputChange`; on rising edge it flips `blanked` and redraws.

**SensESP dependency**: The project references a local `SensESP` library at `../SensESP/` (sibling directory) as well as the published `SensESP@^1.0.8-alpha`. The local path takes precedence.

## OLED Display (HW-364A/HW-364B board)

- SDA: GPIO14 (D6), SCL: GPIO12 (D5) — **note: these are swapped from typical wiring**
- I2C address: 0x3C
- Library: U8g2 (`U8G2_SSD1306_128X64_NONAME_F_HW_I2C`)
