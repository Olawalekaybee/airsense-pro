// =============================================================================
//  display_driver.cpp — LovyanGFX + LVGL integration for 7" Sunton CYD
//  Adapt LGFX_Device pin assignments to your exact board revision.
// =============================================================================

#include "display_driver.h"
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "config.h"

// ---------------------------------------------------------------------------
// LovyanGFX device configuration (RGB parallel interface, 800×480)
// ---------------------------------------------------------------------------
class LGFX_Device : public lgfx::LGFX_Device {
    lgfx::Panel_RGB   _panel;
    lgfx::Bus_RGB     _bus;
    lgfx::Light_PWM   _bl;

public:
    LGFX_Device() {
        // --- Bus (RGB parallel) ---
        {
            auto cfg = _bus.config();
            cfg.panel       = &_panel;
            // Data lines — adjust to your board
            cfg.pin_d0  = 8;   cfg.pin_d1  = 3;   cfg.pin_d2  = 46;  cfg.pin_d3  = 9;
            cfg.pin_d4  = 10;  cfg.pin_d5  = 11;  cfg.pin_d6  = 12;  cfg.pin_d7  = 13;
            cfg.pin_d8  = 14;  cfg.pin_d9  = 21;  cfg.pin_d10 = 47;  cfg.pin_d11 = 48;
            cfg.pin_d12 = 45;  cfg.pin_d13 = 0;   cfg.pin_d14 = 4;   cfg.pin_d15 = 5;
            cfg.pin_henable = 42;
            cfg.pin_vsync   = 41;
            cfg.pin_hsync   = 39;
            cfg.pin_pclk    = 40;
            cfg.freq_write  = 14000000;
            cfg.hsync_polarity   = 0;  cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width= 4;  cfg.hsync_back_porch  = 16;
            cfg.vsync_polarity   = 0;  cfg.vsync_front_porch = 4;
            cfg.vsync_pulse_width= 4;  cfg.vsync_back_porch  = 4;
            cfg.pclk_active_neg  = 1;
            _bus.config(cfg);
        }
        // --- Panel ---
        {
            auto cfg = _panel.config();
            cfg.memory_width  = DISPLAY_WIDTH;
            cfg.memory_height = DISPLAY_HEIGHT;
            cfg.panel_width   = DISPLAY_WIDTH;
            cfg.panel_height  = DISPLAY_HEIGHT;
            _panel.config(cfg);
        }
        // --- Backlight ---
        {
            auto cfg = _bl.config();
            cfg.pin_bl      = 2;
            cfg.invert      = false;
            cfg.freq        = 12000;
            cfg.pwm_channel = 7;
            _bl.config(cfg);
        }
        _panel.setLight(&_bl);
        _panel.setBus(&_bus);
        setPanel(&_panel);
    }
};

static LGFX_Device gfx;

// ---------------------------------------------------------------------------
// LVGL draw buffer (double-buffered, 1/10 screen size each)
// ---------------------------------------------------------------------------
static const size_t BUF_PIXELS = DISPLAY_WIDTH * (DISPLAY_HEIGHT / 10);
static lv_color_t   buf1[BUF_PIXELS];
static lv_color_t   buf2[BUF_PIXELS];
static lv_disp_draw_buf_t drawBuf;

// ---------------------------------------------------------------------------
// LVGL flush callback
// ---------------------------------------------------------------------------
static void lvgl_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    gfx.startWrite();
    gfx.setAddrWindow(area->x1, area->y1, w, h);
    gfx.writePixels((lgfx::rgb565_t*)color_p, w * h, true);
    gfx.endWrite();
    lv_disp_flush_ready(drv);
}

// ---------------------------------------------------------------------------
// LVGL tick — called from a FreeRTOS timer
// ---------------------------------------------------------------------------
static void lvgl_tick_cb(TimerHandle_t) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// ---------------------------------------------------------------------------
void displayDriver_init() {
    gfx.init();
    gfx.setRotation(0);
    gfx.setBrightness(200);

    lv_disp_draw_buf_init(&drawBuf, buf1, buf2, BUF_PIXELS);

    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res   = DISPLAY_WIDTH;
    dispDrv.ver_res   = DISPLAY_HEIGHT;
    dispDrv.flush_cb  = lvgl_flush;
    dispDrv.draw_buf  = &drawBuf;
    lv_disp_drv_register(&dispDrv);

    // Software tick via FreeRTOS timer
    TimerHandle_t tickTimer = xTimerCreate(
        "lvgl_tick", pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS),
        pdTRUE, nullptr, lvgl_tick_cb);
    xTimerStart(tickTimer, 0);

    Serial.printf("[Display] LovyanGFX + LVGL ready (%dx%d)\n",
                  DISPLAY_WIDTH, DISPLAY_HEIGHT);
}
