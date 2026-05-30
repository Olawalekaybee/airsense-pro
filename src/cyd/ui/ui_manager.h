#pragma once
// =============================================================================
//  ui_manager.h — 4-screen LVGL UI for the CYD 7" display
//
//  Screens:
//    0  Dashboard  — live arc gauges for AQI, eCO₂, TVOC, temp, humidity
//    1  History    — scrollable chart of last 60 readings (placeholder)
//    2  Settings   — threshold sliders (placeholder)
//    3  Alerts     — alert log with timestamps
//
//  Navigation: tap the nav bar at the bottom to switch screens.
//  Alert overlay: full-screen red banner shown on top when hasAlert() == true.
// =============================================================================
#include <lvgl.h>
#include "telemetry.h"

// Called once after lv_init() to create all screens
void ui_init();

// Called from DisplayTask with the LVGL mutex held
void ui_updateTelemetry(const TelemetryData* data);
