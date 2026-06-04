---
noteId: "95860c60600111f1b9c8ddd1a9ff75b9"
tags: []

---

# AirSense Pro

**Dual-node IoT air quality monitor** built on Arduino Nano ESP32 + 7" ESP32-S3 CYD display, using PlatformIO, FreeRTOS, MQTT, LVGL, and OTA firmware updates.

---

## What it does

| Node | Role | Key tech |
|------|------|----------|
| Arduino Nano ESP32 | Sensor publisher | ENS160 + AHT21 over I²C, FreeRTOS, MQTT, OTA |
| ESP32-S3 CYD 7" | Touch display subscriber | LVGL 8 UI, GT911 touch, FreeRTOS, MQTT, OTA |

The Nano reads air quality every 10 seconds (eCO₂, TVOC, AQI from ENS160; temperature and humidity from AHT21) and publishes a JSON payload to an MQTT broker. The CYD subscribes, parses the payload, and renders live gauges on a 4-screen LVGL touch dashboard. Both devices support OTA firmware updates from VS Code — no USB cable needed after the first flash.

---

## Sensors

| Sensor | Measurement | Range |
|--------|-------------|-------|
| ScioSense ENS160 | eCO₂ (ppm) | 400 – 65 000 |
| ScioSense ENS160 | TVOC (ppb) | 0 – 65 000 |
| ScioSense ENS160 | AQI (1–5) | 1 = Excellent … 5 = Unhealthy |
| AHT21 | Temperature | −40 – +85 °C |
| AHT21 | Relative humidity | 0 – 100 %RH |

> **ENS160 compensation** — the ENS160 requires the ambient temperature and humidity from the AHT21 as compensation inputs to produce accurate eCO₂ and TVOC readings. The firmware calls `ens160.setTempAndHum()` before each measurement.

---

## Architecture

```
┌──────────────────────────┐         ┌──────────────────────────────────────┐
│  Arduino Nano ESP32      │         │  MQTT Broker (HiveMQ Cloud)          │
│                          │ publish │                                      │
│  SensorTask ──► Queue ──►│────────►│  airsense/room1/telemetry            │
│  MqttTask  ◄── Queue     │         │  airsense/room1/alert                │
│  OtaTask                 │         │  airsense/room1/status (LWT)         │
└──────────────────────────┘         └──────────┬───────────────────────────┘
                                                │ subscribe
                                     ┌──────────▼───────────────────────────┐
                                     │  CYD 7" Display Node                 │
                                     │                                      │
                                     │  MqttTask ──► Queue ──► DisplayTask  │
                                     │  LVGL 4-screen touch UI              │
                                     │  OtaTask                             │
                                     └──────────────────────────────────────┘
                                                │ subscribe
                                     ┌──────────▼────────────┐
                                     │  Node-RED dashboard   │
                                     │  InfluxDB (optional)  │
                                     └───────────────────────┘
```

---

## MQTT topic map

| Topic | Direction | Payload |
|-------|-----------|---------|
| `airsense/room1/telemetry` | Nano → broker | JSON: temp, humidity, eCO₂, tvoc, aqi, alert flags, ts_ms |
| `airsense/room1/alert` | Nano → broker | JSON: type, value, threshold |
| `airsense/room1/status` | Nano → broker | `"online"` / `"offline"` (LWT) |
| `airsense/ota/nano` | broker → Nano | OTA trigger |
| `airsense/ota/cyd` | broker → CYD | OTA trigger |

---

## Repo structure

```
airsense-pro/
├── platformio.ini          # Both environments: [env:nano] and [env:cyd]
├── secrets.ini.template    # Copy → secrets.ini, fill in credentials (gitignored)
├── include/
│   ├── config.h            # All tunable parameters + macro-expanded secrets
│   ├── telemetry.h         # Shared TelemetryData struct
│   └── lv_conf.h           # LVGL 8 build configuration
├── lib/
│   ├── MqttManager/        # Thin PubSubClient wrapper (TLS, LWT, reconnect)
│   └── OtaManager/         # ArduinoOTA wrapper with FreeRTOS task launcher
├── src/
│   ├── nano/               # Nano ESP32 firmware
│   │   ├── main.cpp        # Wi-Fi, task launch
│   │   ├── sensor_task.*   # ENS160 + AHT21 read → queue
│   │   └── mqtt_task.*     # Queue → JSON → MQTT publish
│   └── cyd/                # CYD 7" firmware
│       ├── main.cpp        # Wi-Fi, LVGL init, task launch
│       ├── display_driver.*# LovyanGFX + LVGL flush callback
│       ├── touch_driver.*  # GT911 LVGL indev driver
│       ├── mqtt_task_cyd.* # MQTT subscribe → parse → queue
│       └── ui/
│           ├── ui_manager.h
│           └── ui_manager.cpp  # 4-screen LVGL UI (Dashboard/History/Settings/Alerts)
└── docs/
    ├── wiring.md           # Pin assignments for both nodes
    └── node-red-flow.json  # Import into Node-RED for a live web dashboard
```

---

## Quick start

### 1. Clone and configure

```bash
git clone https://github.com/YOUR_USERNAME/airsense-pro.git
cd airsense-pro
cp secrets.ini.template secrets.ini
# Edit secrets.ini — add Wi-Fi credentials and HiveMQ Cloud details
```

### 2. First flash via USB

```bash
# Nano ESP32 (USB-C)
pio run -e nano-usb -t upload

# CYD 7" (USB-C)
pio run -e cyd-usb -t upload
```

### 3. OTA updates (after first flash)

Update `NANO_IP` and `CYD_IP` in `secrets.ini`, then:

```bash
pio run -e nano -t upload   # OTA to Nano
pio run -e cyd  -t upload   # OTA to CYD
```

### 4. Serial monitor

```bash
pio device monitor -e nano
pio device monitor -e cyd
```

---

## Hardware

| Component | Notes |
|-----------|-------|
| Arduino Nano ESP32 | ~$18 |
| Sunton 7" ESP32-S3 CYD (800×480 capacitive touch) | ~$35 |
| ENS160 + AHT21 combo breakout (e.g. DFRobot SEN0514) | ~$12 |
| Micro buzzer (optional, GPIO 2 on CYD) | ~$1 |

See [`docs/wiring.md`](docs/wiring.md) for full pin assignments.

---

## Thresholds (configurable in `include/config.h`)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `THRESHOLD_ECO2_PPM` | 1000 | eCO₂ alert threshold (ppm) |
| `THRESHOLD_TVOC_PPB` | 500 | TVOC alert threshold (ppb) |
| `THRESHOLD_TEMP_C` | 35.0 | Temperature alert (°C) |
| `THRESHOLD_HUMIDITY` | 80.0 | Humidity alert (%RH) |

---

## Built with

- [PlatformIO](https://platformio.org/)
- [Arduino-ESP32](https://github.com/espressif/arduino-esp32)
- [LVGL 8](https://lvgl.io/)
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [ScioSense ENS160 library](https://github.com/sciosense/ENS160_driver)
- [Adafruit AHTX0](https://github.com/adafruit/Adafruit_AHTX0)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [ArduinoJson 7](https://arduinojson.org/)
- [bb_captouch](https://github.com/bitbank2/bb_captouch)

---

## License

MIT
