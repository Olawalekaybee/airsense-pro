// =============================================================================
//  touch_driver.cpp — GT911 touch via LovyanGFX built-in Touch_GT911
//  LovyanGFX handles GT911 I2C internally via _panel.setTouch() in display_driver.
//  This file just registers the LVGL indev that reads from gfx.getTouch().
// =============================================================================

#include "touch_driver.h"
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lvgl.h>

// Forward declaration — gfx is defined in display_driver.cpp
extern lgfx::LGFX_Device* getGFX();

// We access gfx via a simple extern — declared here, defined in display_driver.cpp
// via a getter to avoid linking issues
static lgfx::v1::LGFX_Device* _gfx = nullptr;

// ---------------------------------------------------------------------------
static void touchpad_read(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    if (!_gfx) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    uint16_t x, y;
    if (_gfx->getTouch(&x, &y)) {
        data->point.x = (int16_t)x;
        data->point.y = (int16_t)y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ---------------------------------------------------------------------------
void touchDriver_init() {
    _gfx = getGFX();

    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type    = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = touchpad_read;
    lv_indev_drv_register(&indevDrv);

    Serial.println("[Touch] GT911 via LovyanGFX ready");
}