#pragma once
#include <stdint.h>

// =============================================================================
//  TelemetryData — the canonical payload structure shared across the project.
//
//  Nano populates all fields and serialises to JSON before publishing.
//  CYD deserialises from JSON into this struct and passes it to LVGL.
// =============================================================================

struct TelemetryData {
    // AHT21
    float    temperature;      // °C
    float    humidity;         // %RH

    // ENS160 (compensated with AHT21 values)
    uint16_t eco2_ppm;         // equivalent CO₂ (400–65000 ppm)
    uint16_t tvoc_ppb;         // total VOCs (0–65000 ppb)
    uint8_t  aqi;              // air quality index 1 (excellent) – 5 (unhealthy)

    // ENS160 status
    uint8_t  ens160_status;    // raw ENS160 STATUS register (0 = OK)

    // Metadata
    uint32_t timestamp_ms;     // millis() on the sender at publish time
    bool     alert_eco2;       // true when eCO₂ exceeds threshold
    bool     alert_tvoc;       // true when TVOC exceeds threshold
    bool     alert_temp;       // true when temperature exceeds threshold
    bool     alert_humidity;   // true when humidity exceeds threshold

    // Convenience
    bool hasAlert() const {
        return alert_eco2 || alert_tvoc || alert_temp || alert_humidity;
    }

    // Human-readable AQI string
    const char* aqiLabel() const {
        switch (aqi) {
            case 1: return "Excellent";
            case 2: return "Good";
            case 3: return "Moderate";
            case 4: return "Poor";
            case 5: return "Unhealthy";
            default: return "Unknown";
        }
    }
};