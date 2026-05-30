// =============================================================================
//  mqtt_task_cyd.cpp — CYD MQTT subscriber
//  Subscribes to airsense/room1/telemetry, deserialises JSON into
//  TelemetryData, and posts to xDisplayQueue for the DisplayTask.
// =============================================================================

#include "mqtt_task_cyd.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/queue.h>
#include "config.h"
#include "telemetry.h"
#include "MqttManager.h"

extern QueueHandle_t xDisplayQueue;

static MqttManager mqtt;

// ---------------------------------------------------------------------------
static void onMqttMessage(const char* topic, const uint8_t* payload, unsigned int len) {
    if (strcmp(topic, TOPIC_TELEMETRY) != 0) return;

    // Parse JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, len);
    if (err) {
        Serial.printf("[MQTT-CYD] JSON parse error: %s\n", err.c_str());
        return;
    }

    TelemetryData data = {};
    data.temperature   = doc["temperature"]  | 0.0f;
    data.humidity      = doc["humidity"]      | 0.0f;
    data.eco2_ppm      = doc["eco2_ppm"]      | (uint16_t)0;
    data.tvoc_ppb      = doc["tvoc_ppb"]      | (uint16_t)0;
    data.aqi           = doc["aqi"]           | (uint8_t)1;
    data.alert_eco2    = doc["alert_eco2"]    | false;
    data.alert_tvoc    = doc["alert_tvoc"]    | false;
    data.alert_temp    = doc["alert_temp"]    | false;
    data.alert_humidity= doc["alert_hum"]     | false;
    data.timestamp_ms  = doc["ts_ms"]         | (uint32_t)0;

    if (xQueueSend(xDisplayQueue, &data, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("[MQTT-CYD] Display queue full — dropping");
    }
}

// ---------------------------------------------------------------------------
void mqttTaskCYD(void* param) {
    char clientId[32];
    snprintf(clientId, sizeof(clientId), "airsense-cyd-%06llX",
             (uint64_t)(ESP.getEfuseMac() & 0xFFFFFF));

    mqtt.begin(clientId, onMqttMessage);
    mqtt.subscribe(TOPIC_TELEMETRY);
    mqtt.subscribe(TOPIC_ALERT);
    mqtt.subscribe(TOPIC_OTA_CYD);

    for (;;) {
        mqtt.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
