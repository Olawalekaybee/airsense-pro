// =============================================================================
//  AirSense Pro — CYD 7" Display Node
//  Firmware entry point: Wi-Fi, LVGL init, FreeRTOS tasks.
//
//  Tasks:
//    MqttTask    — subscribes to airsense/room1/telemetry, pushes parsed
//                  TelemetryData onto xDisplayQueue
//    DisplayTask — consumes queue, updates LVGL widgets thread-safely
//    OtaTask     — ArduinoOTA.handle() in a low-priority loop
//
//  Touch: GT911 capacitive controller, I²C address 0x5D or 0x14.
//         LVGL indev driver polls the controller and maps touch events to
//         screen presses, driving the 4-screen navigation (tap to switch).
// =============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <lvgl.h>

#include "config.h"
#include "telemetry.h"
#include "OtaManager.h"
#include "display_driver.h"   // LovyanGFX + LVGL flush/tick callbacks
#include "touch_driver.h"     // GT911 read callback for LVGL indev
#include "ui/ui_manager.h"    // Screen definitions (Dashboard, History, etc.)
#include "mqtt_task_cyd.h"

// Shared queue — MqttTask produces, DisplayTask consumes
QueueHandle_t xDisplayQueue;

// LVGL mutex — must be held before any lv_* call outside DisplayTask
SemaphoreHandle_t xLvglMutex;

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
static void displayTask(void* param) {
    TelemetryData data;
    for (;;) {
        // Update LVGL tick — must be called regularly even without new data
        if (xSemaphoreTake(xLvglMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(xLvglMutex);
        }

        // Check for new telemetry (non-blocking)
        if (xQueueReceive(xDisplayQueue, &data, 0) == pdTRUE) {
            if (xSemaphoreTake(xLvglMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                ui_updateTelemetry(&data);
                xSemaphoreGive(xLvglMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));   // ~200 Hz LVGL refresh ceiling
    }
}

// --------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.printf("\n=== AirSense Pro | CYD 7\" Display Node | %s ===\n", __DATE__);

    wifiConnect();
    OtaManager::begin();

    // ---- LVGL + display driver ----
    lv_init();
    displayDriver_init();  // LovyanGFX setup + LVGL flush callback
    touchDriver_init();    // GT911 I²C setup + LVGL indev registration

    xLvglMutex = xSemaphoreCreateMutex();
    configASSERT(xLvglMutex);

    // ---- Build UI screens ----
    ui_init();             // creates Dashboard, History, Settings, Alerts screens

    // ---- Inter-task queue ----
    xDisplayQueue = xQueueCreate(QUEUE_SIZE, sizeof(TelemetryData));
    configASSERT(xDisplayQueue);

    // ---- Launch tasks ----
    xTaskCreate(displayTask,   "DisplayTask", STACK_DISPLAY, nullptr, PRIO_DISPLAY, nullptr);
    xTaskCreate(mqttTaskCYD,   "MqttTask",   STACK_MQTT,    nullptr, PRIO_MQTT,    nullptr);
    OtaManager::startTask();

    Serial.println("[Main] All tasks started.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
