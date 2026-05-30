// =============================================================================
//  MqttTask (Nano) — consumes TelemetryData from the queue, serialises to
//  JSON with ArduinoJson, and publishes to the MQTT broker via MqttManager.
//
//  JSON payload example:
//  {
//    "temperature": 24.5,
//    "humidity":    55.2,
//    "eco2_ppm":    612,
//    "tvoc_ppb":    43,
//    "aqi":         2,
//    "aqi_label":   "Good",
//    "alert":       false,
//    "alert_eco2":  false,
//    "alert_tvoc":  false,
//    "alert_temp":  false,
//    "alert_hum":   false,
//    "ts_ms":       123456789,
//    "node":        "NanoSensor"
//  }
// =============================================================================

#include "mqtt_task.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <freertos/queue.h>
#include "config.h"
#include "telemetry.h"
#include "MqttManager.h"

extern QueueHandle_t xTelemetryQueue;

static MqttManager mqtt;

// --------------------------------------------------------------------------
static void onMqttMessage(const char* topic, const uint8_t* payload, unsigned int len) {
    // Nano only listens for OTA trigger — actual OTA is handled by ArduinoOTA
    // via UDP; this topic can carry a "reboot" command for a clean restart.
    if (strcmp(topic, TOPIC_OTA_NANO) == 0) {
        Serial.println("[MQTT] OTA command received — awaiting OTA push");
    }
}

// --------------------------------------------------------------------------
void mqttTaskNano(void* param) {
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "airsense-nano-%06llX",
             (uint64_t)(ESP.getEfuseMac() & 0xFFFFFF));

    mqtt.begin(clientId, onMqttMessage);
    mqtt.subscribe(TOPIC_OTA_NANO);

    TelemetryData data;

    for (;;) {
        mqtt.loop();

        // Block waiting for a reading (up to 1 s, then service MQTT keep-alive)
        if (xQueueReceive(xTelemetryQueue, &data, pdMS_TO_TICKS(1000)) == pdTRUE) {
            // Build JSON document (256-byte stack allocation)
            JsonDocument doc;
            doc["temperature"] = serialized(String(data.temperature, 1));
            doc["humidity"]    = serialized(String(data.humidity,    1));
            doc["eco2_ppm"]    = data.eco2_ppm;
            doc["tvoc_ppb"]    = data.tvoc_ppb;
            doc["aqi"]         = data.aqi;
            doc["aqi_label"]   = data.aqiLabel();
            doc["alert"]       = data.hasAlert();
            doc["alert_eco2"]  = data.alert_eco2;
            doc["alert_tvoc"]  = data.alert_tvoc;
            doc["alert_temp"]  = data.alert_temp;
            doc["alert_hum"]   = data.alert_humidity;
            doc["ts_ms"]       = data.timestamp_ms;
            doc["node"]        = NODE_NAME;

            char buf[384];
            size_t n = serializeJson(doc, buf, sizeof(buf));
            if (n == 0) {
                Serial.println("[MQTT] JSON serialisation failed");
                continue;
            }

            // Publish telemetry
            mqtt.publish(TOPIC_TELEMETRY, buf);

            // Publish alert to separate topic if any threshold is breached
            if (data.hasAlert()) {
                JsonDocument alertDoc;
                if (data.alert_eco2)     { alertDoc["type"] = "eco2";     alertDoc["value"] = data.eco2_ppm;     alertDoc["threshold"] = THRESHOLD_ECO2_PPM; }
                else if (data.alert_tvoc){ alertDoc["type"] = "tvoc";     alertDoc["value"] = data.tvoc_ppb;     alertDoc["threshold"] = THRESHOLD_TVOC_PPB; }
                else if (data.alert_temp){ alertDoc["type"] = "temp";     alertDoc["value"] = data.temperature;  alertDoc["threshold"] = THRESHOLD_TEMP_C;   }
                else                     { alertDoc["type"] = "humidity"; alertDoc["value"] = data.humidity;     alertDoc["threshold"] = THRESHOLD_HUMIDITY; }

                char alertBuf[128];
                serializeJson(alertDoc, alertBuf, sizeof(alertBuf));
                mqtt.publish(TOPIC_ALERT, alertBuf);
            }
        }
    }
}
