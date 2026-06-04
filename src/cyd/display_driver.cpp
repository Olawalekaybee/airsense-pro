// =============================================================================
//  display_driver.cpp — LovyanGFX + LVGL 9 for Sunton ESP32-8048S070 (7" 800x480)
//
//  LVGL 9 API changes from LVGL 8:
//  - lv_disp_drv_t / lv_disp_drv_register()  → lv_display_create()
//  - lv_disp_draw_buf_t / lv_disp_draw_buf_init() → lv_display_set_buffers()
//  - lv_display_set_flush_cb() instead of flush_cb in drv struct
//  - flush callback signature: (lv_display_t*, const lv_area_t*, uint8_t*)
//  - lv_disp_flush_ready(disp) → lv_display_flush_ready(disp)
//  - lv_tick_set_cb(millis_cb) instead of FreeRTOS timer
// =============================================================================

#include "display_driver.h"
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <driver/i2c.h>
#include "config.h"

class LGFX_Sunton7 : public lgfx::LGFX_Device {
public:
    lgfx::Bus_RGB     _bus;
    lgfx::Panel_RGB   _panel;
    lgfx::Light_PWM   _bl;
    lgfx::Touch_GT911 _touch;

    LGFX_Sunton7() {
        // --- Panel ---
        {
            auto cfg = _panel.config();
            cfg.memory_width  = 800;
            cfg.memory_height = 480;
            cfg.panel_width   = 800;
            cfg.panel_height  = 480;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            _panel.config(cfg);
        }
        // --- PSRAM framebuffer ---
        {
            auto cfg = _panel.config_detail();
            cfg.use_psram = 1;
            _panel.config_detail(cfg);
        }
        // --- RGB Bus ---
        {
            auto cfg = _bus.config();
            cfg.panel = &_panel;

            // Blue B0-B4 → pin_d0 to pin_d4
            cfg.pin_d0  = GPIO_NUM_15;
            cfg.pin_d1  = GPIO_NUM_7;
            cfg.pin_d2  = GPIO_NUM_6;
            cfg.pin_d3  = GPIO_NUM_5;
            cfg.pin_d4  = GPIO_NUM_4;
            // Green G0-G5 → pin_d5 to pin_d10
            cfg.pin_d5  = GPIO_NUM_9;
            cfg.pin_d6  = GPIO_NUM_46;
            cfg.pin_d7  = GPIO_NUM_3;
            cfg.pin_d8  = GPIO_NUM_8;
            cfg.pin_d9  = GPIO_NUM_16;
            cfg.pin_d10 = GPIO_NUM_1;
            // Red R0-R4 → pin_d11 to pin_d15
            cfg.pin_d11 = GPIO_NUM_14;
            cfg.pin_d12 = GPIO_NUM_21;
            cfg.pin_d13 = GPIO_NUM_47;
            cfg.pin_d14 = GPIO_NUM_48;
            cfg.pin_d15 = GPIO_NUM_45;

            cfg.pin_henable = GPIO_NUM_41;
            cfg.pin_vsync   = GPIO_NUM_40;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_42;
            cfg.freq_write  = 12000000;

            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 2;
            cfg.hsync_back_porch  = 43;
            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 2;
            cfg.vsync_back_porch  = 12;
            cfg.pclk_idle_high    = 1;

            _bus.config(cfg);
        }
        _panel.setBus(&_bus);

        // --- Backlight ---
        {
            auto cfg = _bl.config();
            cfg.pin_bl = GPIO_NUM_2;
            _bl.config(cfg);
        }
        _panel.light(&_bl);

        // --- Touch GT911 via LovyanGFX ---
        {
            auto cfg = _touch.config();
            cfg.x_min           = 0;
            cfg.y_min           = 0;
            cfg.x_max           = 800;
            cfg.y_max           = 480;
            cfg.bus_shared      = false;
            cfg.offset_rotation = 0;
            cfg.i2c_port        = I2C_NUM_0;
            cfg.pin_sda         = GPIO_NUM_19;
            cfg.pin_scl         = GPIO_NUM_20;
            cfg.pin_int         = GPIO_NUM_NC;
            cfg.pin_rst         = GPIO_NUM_38;
            cfg.freq            = 100000;
            _touch.config(cfg);
            _panel.setTouch(&_touch);
        }

        setPanel(&_panel);
    }
};

static LGFX_Sunton7 gfx;

// Accessor for touch_driver.cpp
lgfx::LGFX_Device* getGFX() { return &gfx; }

// ---------------------------------------------------------------------------
// LVGL 9 draw buffers — in PSRAM
// ---------------------------------------------------------------------------
static const size_t BUF_PIXELS = 800 * 40;
static lv_color_t*  buf1       = nullptr;
static lv_color_t*  buf2       = nullptr;

// ---------------------------------------------------------------------------
// LVGL 9 flush callback — new signature: (lv_display_t*, area, uint8_t*)
// ---------------------------------------------------------------------------
static void lvgl_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    gfx.startWrite();
    gfx.setAddrWindow(area->x1, area->y1, w, h);
    gfx.writePixels((lgfx::rgb565_t*)px_map, w * h, true);
    gfx.endWrite();
    lv_display_flush_ready(disp);   // LVGL 9: lv_display_flush_ready (not lv_disp_flush_ready)
}

// ---------------------------------------------------------------------------
// LVGL 9 tick — use lv_tick_set_cb(millis_cb) instead of FreeRTOS timer
// ---------------------------------------------------------------------------
static uint32_t millis_cb(void) { return (uint32_t)millis(); }

// ---------------------------------------------------------------------------
void displayDriver_init() {
    gfx.init();
    gfx.setRotation(0);
    gfx.fillScreen(TFT_BLACK);
    Serial.println("[Display] GFX init done, backlight ON");

    // Allocate draw buffers in PSRAM
    buf1 = (lv_color_t*)heap_caps_malloc(BUF_PIXELS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t*)heap_caps_malloc(BUF_PIXELS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf1 || !buf2) {
        Serial.println("[Display] PSRAM alloc failed — using DRAM");
        static lv_color_t fb1[800 * 10], fb2[800 * 10];
        buf1 = fb1; buf2 = fb2;
    }

    // LVGL 9: lv_tick_set_cb replaces the FreeRTOS timer approach
    lv_tick_set_cb(millis_cb);

    // LVGL 9: lv_display_create + lv_display_set_buffers
    lv_display_t* disp = lv_display_create(800, 480);
    lv_display_set_flush_cb(disp, lvgl_flush);
    lv_display_set_buffers(disp, buf1, buf2,
                           BUF_PIXELS * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    Serial.println("[Display] LVGL 9 ready (800x480)");
}