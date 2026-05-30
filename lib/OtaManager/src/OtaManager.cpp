#include "OtaManager.h"

void OtaManager::begin() {
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASS);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.printf("[OTA] Start updating %s\n", type.c_str());
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Complete — rebooting");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("[OTA] %u%%\r", progress * 100 / total);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]: ", error);
        if      (error == OTA_AUTH_ERROR)    Serial.println("Auth failed");
        else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive failed");
        else if (error == OTA_END_ERROR)     Serial.println("End failed");
    });

    ArduinoOTA.begin();
    Serial.printf("[OTA] Ready — hostname: %s\n", OTA_HOSTNAME);
}

void OtaManager::startTask() {
    xTaskCreate(
        _otaTask,
        "OtaTask",
        4096,   // stack (was STACK_OTA macro — lib/ can't see config.h)
        nullptr,
        1,      // priority (was PRIO_OTA macro)
        nullptr
    );
}

void OtaManager::_otaTask(void* param) {
    for (;;) {
        ArduinoOTA.handle();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}