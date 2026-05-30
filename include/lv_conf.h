/**
 * lv_conf.h — LVGL 8.x configuration for AirSense Pro CYD node
 * Place this file where the compiler can find it (lib_deps search path).
 * Key settings tuned for 800×480, 16-bit colour, 8 MB PSRAM.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Colour depth (16-bit RGB565 for most ESP32-S3 CYD panels) */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/* Memory — use PSRAM for LVGL heap */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE   (256 * 1024U)   /* 256 KB internal SRAM for LVGL */

/* Tick */
#define LV_TICK_CUSTOM 0

/* Feature toggles */
#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN

/* Fonts — include the sizes we use in ui_manager.cpp */
#define LV_FONT_MONTSERRAT_10  1
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_48  1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Widgets */
#define LV_USE_ARC      1
#define LV_USE_BTN      1
#define LV_USE_LABEL    1
#define LV_USE_LIST     1
#define LV_USE_CHART    1
#define LV_USE_SLIDER   1
#define LV_USE_SWITCH   1
#define LV_USE_MSGBOX   1
#define LV_USE_TABVIEW  0   /* using manual nav bar instead */

/* Animation */
#define LV_USE_ANIMATION 1

#endif /* LV_CONF_H */