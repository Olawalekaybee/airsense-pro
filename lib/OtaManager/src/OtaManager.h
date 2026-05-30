#pragma once
// =============================================================================
//  OtaManager — wraps ArduinoOTA with a FreeRTOS-friendly task launcher
//  Both nodes include this library and call OtaManager::startTask().
// =============================================================================
#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"

class OtaManager {
public:
    // Call once from setup() — configures ArduinoOTA hostname + password
    static void begin();

    // Launch a dedicated FreeRTOS task that calls ArduinoOTA.handle() in a loop
    static void startTask();

private:
    static void _otaTask(void* param);
};