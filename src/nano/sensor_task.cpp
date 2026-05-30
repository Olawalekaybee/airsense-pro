// =============================================================================
//  SensorTask — reads ENS160 + AHT21, applies compensation, pushes to queue
//
//  Library: dfrobot/DFRobot_ENS160
//  ENS160 requires AHT21 temp+humidity compensation for accurate readings.
//  Always call setTempAndHum() before reading output registers.
// =============================================================================

#include "sensor_task.h"
#include <Arduino.h>
#include <Wire.h>
#include <freertos/queue.h>
#include "DFRobot_ENS160.h"
#include <Adafruit_AHTX0.h>
#include "config.h"
#include "telemetry.h"

extern QueueHandle_t xTelemetryQueue;

// ENS160 on I2C address 0x53 (ADDR pin LOW — default for most breakouts)
static DFRobot_ENS160_I2C ens160(&Wire, 0x53);
static Adafruit_AHTX0     aht21;

// ---------------------------------------------------------------------------
static bool initSensors() {
    bool ok = true;

    // --- AHT21 ---
    if (!aht21.begin()) {
        Serial.println("[Sensor] AHT21 not found — check SDA/SCL wiring");
        ok = false;
    } else {
        Serial.println("[Sensor] AHT21 ready");
    }

    // --- ENS160 ---
    if (ens160.begin() != NO_ERR) {
        Serial.println("[Sensor] ENS160 not found — check SDA/SCL wiring");
        ok = false;
    } else {
        ens160.setPWRMode(ENS160_STANDARD_MODE);
        Serial.println("[Sensor] ENS160 ready — standard mode");
    }

    return ok;
}

// ---------------------------------------------------------------------------
void sensorTask(void* param) {
    // Retry until both sensors respond
    while (!initSensors()) {
        Serial.println("[Sensor] Retrying sensor init in 5 s...");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        TelemetryData data = {};
        data.timestamp_ms = millis();

        // --- Read AHT21 first (needed for ENS160 compensation) ---
        sensors_event_t humEvent, tempEvent;
        if (aht21.getEvent(&humEvent, &tempEvent)) {
            data.temperature = tempEvent.temperature;
            data.humidity    = humEvent.relative_humidity;
        } else {
            Serial.println("[Sensor] AHT21 read failed");
        }

        // --- Compensate ENS160 with live temp + humidity (critical for accuracy) ---
        ens160.setTempAndHum(data.temperature, data.humidity);

        // --- Read ENS160 ---
        data.ens160_status = ens160.getENS160Status();
        data.aqi           = ens160.getAQI();
        data.tvoc_ppb      = ens160.getTVOC();
        data.eco2_ppm      = ens160.getECO2();

        // --- Threshold checks ---
        data.alert_eco2     = (data.eco2_ppm    > THRESHOLD_ECO2_PPM);
        data.alert_tvoc     = (data.tvoc_ppb    > THRESHOLD_TVOC_PPB);
        data.alert_temp     = (data.temperature > THRESHOLD_TEMP_C);
        data.alert_humidity = (data.humidity    > THRESHOLD_HUMIDITY);

        // --- Serial log ---
        Serial.printf("[Sensor] T=%.1f C  RH=%.1f%%  eCO2=%u ppm  TVOC=%u ppb  AQI=%u (%s)%s\n",
            data.temperature, data.humidity,
            data.eco2_ppm, data.tvoc_ppb, data.aqi, data.aqiLabel(),
            data.hasAlert() ? "  *** ALERT ***" : "");

        // --- Push to queue (non-blocking — drop if full) ---
        if (xQueueSend(xTelemetryQueue, &data, 0) != pdTRUE) {
            Serial.println("[Sensor] Queue full — dropping reading");
        }

        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
    }
}