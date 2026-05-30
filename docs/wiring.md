# Wiring reference — AirSense Pro

## Nano ESP32 → ENS160 + AHT21 (I²C)

Both sensors share the same I²C bus.
The ENS160+AHT21 combo breakout (e.g. DFRobot SEN0514) exposes a single 4-pin header.

| Sensor pin | Nano ESP32 pin | Notes                        |
|------------|----------------|------------------------------|
| VCC        | 3V3            | 3.3 V only — NOT 5 V         |
| GND        | GND            |                              |
| SDA        | A4 (GPIO 11)   | Default Wire SDA on Nano ESP32 |
| SCL        | A5 (GPIO 12)   | Default Wire SCL on Nano ESP32 |

I²C addresses:
- ENS160 → 0x53 (ADDR pin low, default)
- AHT21  → 0x38 (fixed)

**No pull-up resistors needed** — the Nano ESP32 enables internal pull-ups via Wire.begin().

---

## CYD 7" (Sunton ESP32-S3) — built-in display + touch

The 7" Sunton board has the display, touch controller (GT911), and SD card
all on-board. No additional wiring is needed for the display.

Touch I²C pins (GT911) — set in config via build flags:

| Signal   | GPIO |
|----------|------|
| SDA      | 19   |
| SCL      | 20   |
| INT      | 18   |
| RST      | 38   |

Buzzer (optional — alert feedback):

| Signal   | GPIO |
|----------|------|
| Buzzer + | 2    |
| Buzzer − | GND  |

---

## Power

| Device      | Supply               |
|-------------|----------------------|
| Nano ESP32  | USB-C or Vin 5 V     |
| CYD 7"      | USB-C (5 V / 2 A min)|
| ENS160+AHT21| From Nano 3V3 pin    |
