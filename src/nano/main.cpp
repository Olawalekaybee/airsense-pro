// =============================================================================
//  AirSense Pro — Nano ESP32 Sensor Node
//  Firmware entry point: sets up Wi-Fi, launches FreeRTOS tasks.
//
//  Tasks:
//    SensorTask  — reads ENS160 + AHT21 every SENSOR_INTERVAL_MS, pushes
//                  TelemetryData onto xTelemetryQueue
//    MqttTask    — consumes queue, serialises to JSON, publishes to broker
//    OtaTask     — calls ArduinoOTA.handle() in a tight loop (low priority)
// =============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "config.h"
#include "telemetry.h"
#include "MqttManager.h"
#include "OtaManager.h"
#include "sensor_task.h"
#include "mqtt_task.h"

// Shared queue — SensorTask produces, MqttTask consumes
QueueHandle_t xTelemetryQueue;

// --------------------------------------------------------------------------
static void wifiConnect() {
    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Serial.println("\n[WiFi] Timeout — restarting");
            ESP.restart();
        }
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Connected — IP: %s\n", WiFi.localIP().toString().c_str());
}

// --------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.printf("\n=== AirSense Pro | Nano Sensor Node | %s ===\n", __DATE__);

    Wire.begin();       // default I²C pins for Nano ESP32 (SDA=A4, SCL=A5)
    wifiConnect();

    OtaManager::begin();

    // Create the inter-task queue
    xTelemetryQueue = xQueueCreate(QUEUE_SIZE, sizeof(TelemetryData));
    configASSERT(xTelemetryQueue);

    // Launch tasks — SensorTask and MqttTask are defined in their own files
    xTaskCreate(sensorTask, "SensorTask", STACK_SENSOR, nullptr, PRIO_SENSOR, nullptr);
    xTaskCreate(mqttTaskNano, "MqttTask",  STACK_MQTT,  nullptr, PRIO_MQTT,  nullptr);
    OtaManager::startTask();

    Serial.println("[Main] All tasks started.");
    // setup() returns — FreeRTOS scheduler takes over
}

void loop() {
    // Intentionally empty — FreeRTOS tasks own execution.
    // The idle task runs here; avoid blocking calls.
    vTaskDelay(portMAX_DELAY);
}
