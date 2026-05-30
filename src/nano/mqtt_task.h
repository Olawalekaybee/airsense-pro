#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// FreeRTOS task function — launched from main.cpp
void mqttTaskNano(void* param);
