// =============================================================================
//  touch_driver.cpp — GT911 capacitive touch controller for LVGL
//  Uses bb_captouch library for simple I²C read.
//  Registers an LVGL input device driver so LVGL handles all touch routing.
// =============================================================================

#include "touch_driver.h"
#include <Arduino.h>
#include <Wire.h>
#include <bb_captouch.h>
#include <lvgl.h>
#include "config.h"

static BBCapTouch touch;

// ---------------------------------------------------------------------------
// LVGL input device read callback — called every LVGL tick
// ---------------------------------------------------------------------------
static void touchpad_read(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    TOUCHINFO ti;
    if (touch.getSamples(&ti) && ti.count > 0) {
        // GT911 reports raw coordinates; map to display resolution
        data->point.x = (int16_t)map(ti.x[0], 0, 4096, 0, DISPLAY_WIDTH);
        data->point.y = (int16_t)map(ti.y[0], 0, 4096, 0, DISPLAY_HEIGHT);
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ---------------------------------------------------------------------------
void touchDriver_init() {
    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    if (!touch.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT)) {
        Serial.println("[Touch] GT911 not found — touch disabled");
        return;
    }
    Serial.println("[Touch] GT911 ready");

    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type    = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = touchpad_read;
    lv_indev_drv_register(&indevDrv);
}
