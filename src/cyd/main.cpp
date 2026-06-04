// =============================================================================
//  AirSense Pro — CYD 7" Display Node  (LVGL 9)
//
//  LVGL 9 change: lv_timer_handler() is unchanged.
//  lv_init() must be called before displayDriver_init().
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
#include "display_driver.h"
#include "touch_driver.h"
#include "ui/ui_manager.h"
#include "mqtt_task_cyd.h"

QueueHandle_t    xDisplayQueue;
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
        if (xSemaphoreTake(xLvglMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler();   // same in LVGL 9
            xSemaphoreGive(xLvglMutex);
        }
        if (xQueueReceive(xDisplayQueue, &data, 0) == pdTRUE) {
            if (xSemaphoreTake(xLvglMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                ui_updateTelemetry(&data);
                xSemaphoreGive(xLvglMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.printf("\n=== AirSense Pro | CYD 7\" Display Node | %s ===\n", __DATE__);

    wifiConnect();
    OtaManager::begin();

    // LVGL 9: lv_init() first, then display + touch drivers
    lv_init();
    displayDriver_init();
    touchDriver_init();

    xLvglMutex = xSemaphoreCreateMutex();
    configASSERT(xLvglMutex);

    ui_init();

    xDisplayQueue = xQueueCreate(QUEUE_SIZE, sizeof(TelemetryData));
    configASSERT(xDisplayQueue);

    xTaskCreate(displayTask, "DisplayTask", STACK_DISPLAY, nullptr, PRIO_DISPLAY, nullptr);
    xTaskCreate(mqttTaskCYD, "MqttTask",   STACK_MQTT,    nullptr, PRIO_MQTT,    nullptr);
    OtaManager::startTask();

    Serial.println("[Main] All tasks started.");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}