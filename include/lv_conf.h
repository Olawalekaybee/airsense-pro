#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH   16
#define LV_COLOR_16_SWAP  0

/* Use custom malloc so LVGL heap lives in PSRAM */
#define LV_MEM_CUSTOM      1
#define LV_MEM_CUSTOM_INCLUDE <esp_heap_caps.h>
#define LV_MEM_CUSTOM_ALLOC(size)       heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LV_MEM_CUSTOM_FREE(p)           heap_caps_free(p)
#define LV_MEM_CUSTOM_REALLOC(p, size)  heap_caps_realloc(p, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

#define LV_TICK_CUSTOM 0

#define LV_USE_LOG   1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* Fonts */
#define LV_FONT_MONTSERRAT_10  1
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_48  1
#define LV_FONT_DEFAULT        &lv_font_montserrat_14

/* Widgets */
#define LV_USE_ARC      1
#define LV_USE_BTN      1
#define LV_USE_LABEL    1
#define LV_USE_LIST     1
#define LV_USE_CHART    1
#define LV_USE_SLIDER   1
#define LV_USE_SWITCH   1
#define LV_USE_MSGBOX   1

/* Layouts */
#define LV_USE_FLEX  1
#define LV_USE_GRID  1

/* Animation */
#define LV_USE_ANIMATION 1

#endif /* LV_CONF_H */