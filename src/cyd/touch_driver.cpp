// =============================================================================
//  touch_driver.cpp — GT911 touch via LovyanGFX, LVGL 9 indev API
//
//  LVGL 9 API changes from LVGL 8:
//  - lv_indev_drv_t / lv_indev_drv_register() → lv_indev_create()
//  - lv_indev_set_type() / lv_indev_set_read_cb() instead of struct fields
//  - lv_indev_data_t unchanged (still has .point and .state)
// =============================================================================

#include "touch_driver.h"
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lvgl.h>

extern lgfx::LGFX_Device* getGFX();
static lgfx::v1::LGFX_Device* _gfx = nullptr;

// ---------------------------------------------------------------------------
// LVGL 9 read callback — signature unchanged from LVGL 8
// ---------------------------------------------------------------------------
static void touchpad_read(lv_indev_t* indev, lv_indev_data_t* data) {
    if (!_gfx) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    uint16_t x, y;
    if (_gfx->getTouch(&x, &y)) {
        data->point.x = (int32_t)x;
        data->point.y = (int32_t)y;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ---------------------------------------------------------------------------
void touchDriver_init() {
    _gfx = getGFX();

    // LVGL 9: lv_indev_create() + lv_indev_set_type() + lv_indev_set_read_cb()
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read);

    Serial.println("[Touch] GT911 via LovyanGFX (LVGL 9) ready");
}