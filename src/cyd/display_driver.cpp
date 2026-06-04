// =============================================================================
//  display_driver.cpp — LovyanGFX 1.2.x + LVGL 8 for 7" Sunton ESP32-S3
//  LVGL draw buffers allocated in PSRAM (ps_malloc) to avoid DRAM overflow.
//  The board has 8MB PSRAM — buffers of ~75KB each fit comfortably.
// =============================================================================

#include "display_driver.h"
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include "config.h"

// ---------------------------------------------------------------------------
// LovyanGFX device
// ---------------------------------------------------------------------------
class LGFX_Sunton7 : public lgfx::LGFX_Device {
public:
    lgfx::v1::Bus_RGB   _bus;
    lgfx::v1::Panel_RGB _panel;
    lgfx::v1::Light_PWM _bl;

    LGFX_Sunton7() {
        // --- Bus ---
        {
            auto cfg = _bus.config();
            cfg.panel = &_panel;
            cfg.pin_d0  = 8;  cfg.pin_d1  = 3;  cfg.pin_d2  = 46; cfg.pin_d3  = 9;
            cfg.pin_d4  = 10; cfg.pin_d5  = 11; cfg.pin_d6  = 12; cfg.pin_d7  = 13;
            cfg.pin_d8  = 14; cfg.pin_d9  = 21; cfg.pin_d10 = 47; cfg.pin_d11 = 48;
            cfg.pin_d12 = 45; cfg.pin_d13 = 0;  cfg.pin_d14 = 4;  cfg.pin_d15 = 5;
            cfg.pin_henable = 42;
            cfg.pin_vsync   = 41;
            cfg.pin_hsync   = 39;
            cfg.pin_pclk    = 40;
            cfg.freq_write  = 14000000;
            cfg.hsync_polarity    = 0; cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 4; cfg.hsync_back_porch  = 16;
            cfg.vsync_polarity    = 0; cfg.vsync_front_porch = 4;
            cfg.vsync_pulse_width = 4; cfg.vsync_back_porch  = 4;
            cfg.pclk_active_neg   = 1;
            _bus.config(cfg);
        }
        // --- Panel ---
        {
            auto cfg = _panel.config();
            cfg.memory_width  = DISPLAY_WIDTH;
            cfg.memory_height = DISPLAY_HEIGHT;
            cfg.panel_width   = DISPLAY_WIDTH;
            cfg.panel_height  = DISPLAY_HEIGHT;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            _panel.config(cfg);
            _panel.setBus(&_bus);
        }
        // --- Backlight ---
        {
            auto cfg = _bl.config();
            cfg.pin_bl      = 2;
            cfg.invert      = false;
            cfg.freq        = 12000;
            cfg.pwm_channel = 7;
            _bl.config(cfg);
            _panel.setLight(&_bl);
        }
        setPanel(&_panel);
    }
};

static LGFX_Sunton7 gfx;

// ---------------------------------------------------------------------------
// LVGL draw buffers — in PSRAM to avoid DRAM overflow
// 1/10 screen = 800 * 48 = 38400 px * 2 bytes = ~75 KB each
// ---------------------------------------------------------------------------
static const size_t      BUF_PIXELS = DISPLAY_WIDTH * (DISPLAY_HEIGHT / 10);
static lv_color_t*       buf1 = nullptr;
static lv_color_t*       buf2 = nullptr;
static lv_disp_draw_buf_t drawBuf;

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

static void lvgl_tick_cb(TimerHandle_t) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

// ---------------------------------------------------------------------------
void displayDriver_init() {
    gfx.init();
    gfx.setRotation(0);
    gfx.setBrightness(200);

    // Allocate LVGL buffers in PSRAM
    buf1 = (lv_color_t*)heap_caps_malloc(BUF_PIXELS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t*)heap_caps_malloc(BUF_PIXELS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!buf1 || !buf2) {
        Serial.println("[Display] FATAL: PSRAM alloc failed — check board has PSRAM");
        // Fall back to smaller single buffer in DRAM
        static lv_color_t fallbackBuf[DISPLAY_WIDTH * 10];
        lv_disp_draw_buf_init(&drawBuf, fallbackBuf, nullptr, DISPLAY_WIDTH * 10);
    } else {
        lv_disp_draw_buf_init(&drawBuf, buf1, buf2, BUF_PIXELS);
    }

    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res  = DISPLAY_WIDTH;
    dispDrv.ver_res  = DISPLAY_HEIGHT;
    dispDrv.flush_cb = lvgl_flush;
    dispDrv.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDrv);

    TimerHandle_t t = xTimerCreate("lvgl_tick",
        pdMS_TO_TICKS(LVGL_TICK_PERIOD_MS), pdTRUE, nullptr, lvgl_tick_cb);
    xTimerStart(t, 0);

    Serial.printf("[Display] Ready (%dx%d) buf in PSRAM\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);
}