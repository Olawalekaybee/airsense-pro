#pragma once
// =============================================================================
//  AirSense Pro — shared compile-time configuration
//  All user-tunable parameters live here so you never hunt through .cpp files.
//  Credentials are injected via platformio.ini build_flags (from secrets.ini).
// =============================================================================

// ---------- Wi-Fi -----------------------------------------------------------
#ifndef WIFI_SSID
  #error "WIFI_SSID not defined. Did you create secrets.ini from the template?"
#endif
#ifndef WIFI_PASS
  #error "WIFI_PASS not defined."
#endif

// ---------- MQTT ------------------------------------------------------------
#ifndef MQTT_HOST
  #error "MQTT_HOST not defined."
#endif
#ifndef MQTT_PORT
  #define MQTT_PORT 8883
#endif
#ifndef MQTT_USER
  #define MQTT_USER ""
#endif
#ifndef MQTT_PASS
  #define MQTT_PASS ""
#endif

// ---------- Topics ----------------------------------------------------------
#ifndef MQTT_TOPIC_ROOT
  #define MQTT_TOPIC_ROOT "airsense/room1"
#endif

#define TOPIC_TELEMETRY   MQTT_TOPIC_ROOT "/telemetry"
#define TOPIC_ALERT       MQTT_TOPIC_ROOT "/alert"
#define TOPIC_STATUS      MQTT_TOPIC_ROOT "/status"
#define TOPIC_OTA_NANO    "airsense/ota/nano"
#define TOPIC_OTA_CYD     "airsense/ota/cyd"

// ---------- OTA -------------------------------------------------------------
#ifndef OTA_HOSTNAME
  #define OTA_HOSTNAME "airsense-node"
#endif
#ifndef OTA_PASS
  #define OTA_PASS "changeme"
#endif

// ---------- Sensor thresholds (Nano node) -----------------------------------
#define THRESHOLD_ECO2_PPM   1000   // eCO₂ alert above this (ppm)
#define THRESHOLD_TVOC_PPB   500    // TVOC alert above this (ppb)
#define THRESHOLD_TEMP_C     35.0f  // Temperature alert (°C)
#define THRESHOLD_HUMIDITY   80.0f  // Relative humidity alert (%)

// ---------- Timing ----------------------------------------------------------
#ifndef SENSOR_INTERVAL_MS
  #define SENSOR_INTERVAL_MS 10000  // Sensor read + publish interval
#endif
#define MQTT_RECONNECT_DELAY_MS 5000
#define WIFI_TIMEOUT_MS         30000

// ---------- FreeRTOS task stack sizes ---------------------------------------
#define STACK_SENSOR   4096
#define STACK_MQTT     6144
#define STACK_DISPLAY  8192   // CYD only — LVGL needs more headroom
#define STACK_OTA      4096

// ---------- FreeRTOS task priorities (higher = more urgent) ----------------
#define PRIO_SENSOR    2
#define PRIO_MQTT      3
#define PRIO_DISPLAY   2
#define PRIO_OTA       1

// ---------- FreeRTOS queue --------------------------------------------------
#define QUEUE_SIZE     5    // telemetry structs buffered between tasks