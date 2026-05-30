// =============================================================================
//  ui_manager.cpp — LVGL 4-screen touch UI
//  Dashboard | History | Settings | Alerts
// =============================================================================

#include "ui_manager.h"
#include <Arduino.h>
#include <lvgl.h>
#include "config.h"
#include "telemetry.h"

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------
static lv_obj_t* scr_dashboard;
static lv_obj_t* scr_history;
static lv_obj_t* scr_settings;
static lv_obj_t* scr_alerts;

// ---------------------------------------------------------------------------
// Dashboard widgets (updated by ui_updateTelemetry)
// ---------------------------------------------------------------------------
static lv_obj_t* arc_aqi;
static lv_obj_t* lbl_aqi_val;
static lv_obj_t* lbl_aqi_text;
static lv_obj_t* lbl_eco2;
static lv_obj_t* lbl_tvoc;
static lv_obj_t* lbl_temp;
static lv_obj_t* lbl_hum;
static lv_obj_t* lbl_status;
static lv_obj_t* lbl_last_update;

// Alert overlay (shown on top of any screen)
static lv_obj_t* alert_overlay;
static lv_obj_t* lbl_alert_msg;

// Alert log list (on Alerts screen)
static lv_obj_t* alert_list;

// ---------------------------------------------------------------------------
// Colour helpers
// ---------------------------------------------------------------------------
static lv_color_t aqiColor(uint8_t aqi) {
    switch (aqi) {
        case 1: return lv_color_make(0x1D, 0x9E, 0x75);  // teal  — Excellent
        case 2: return lv_color_make(0x63, 0x99, 0x22);  // green — Good
        case 3: return lv_color_make(0xBA, 0x75, 0x17);  // amber — Moderate
        case 4: return lv_color_make(0xD8, 0x5A, 0x30);  // coral — Poor
        case 5: return lv_color_make(0xE2, 0x4B, 0x4A);  // red   — Unhealthy
        default:return lv_color_make(0x88, 0x87, 0x80);  // grey  — Unknown
    }
}

// ---------------------------------------------------------------------------
// Nav bar (shared across all screens)
// ---------------------------------------------------------------------------
static void makeNavBar(lv_obj_t* parent, int activeIdx) {
    const char* labels[] = { LV_SYMBOL_HOME " Dashboard",
                              LV_SYMBOL_CHART " History",
                              LV_SYMBOL_SETTINGS " Settings",
                              LV_SYMBOL_WARNING " Alerts" };
    lv_obj_t* nav = lv_obj_create(parent);
    lv_obj_set_size(nav, DISPLAY_WIDTH, 52);
    lv_obj_align(nav, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(nav, lv_color_make(0x2C, 0x2C, 0x2A), 0);
    lv_obj_set_style_border_width(nav, 0, 0);
    lv_obj_set_style_pad_all(nav, 4, 0);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* screens[] = { scr_dashboard, scr_history, scr_settings, scr_alerts };

    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_btn_create(nav);
        lv_obj_set_size(btn, 180, 44);
        bool active = (i == activeIdx);
        lv_obj_set_style_bg_color(btn, active
            ? lv_color_make(0x53, 0x4A, 0xB7)
            : lv_color_make(0x44, 0x44, 0x41), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 8, 0);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(lbl);

        // Capture screen pointer for the click handler
        lv_obj_t* targetScr = screens[i];
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* scr = (lv_obj_t*)lv_event_get_user_data(e);
            lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
        }, LV_EVENT_CLICKED, targetScr);
    }
}

// ---------------------------------------------------------------------------
// Build Dashboard screen
// ---------------------------------------------------------------------------
static void buildDashboard() {
    scr_dashboard = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_dashboard, lv_color_make(0x1A, 0x1A, 0x18), 0);

    // Title bar
    lv_obj_t* title = lv_label_create(scr_dashboard);
    lv_label_set_text(title, "AirSense Pro");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_make(0xCE, 0xCB, 0xF6), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lbl_status = lv_label_create(scr_dashboard);
    lv_label_set_text(lbl_status, LV_SYMBOL_WIFI " Waiting...");
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_make(0x88, 0x87, 0x80), 0);
    lv_obj_align(lbl_status, LV_ALIGN_TOP_RIGHT, -16, 16);

    // AQI arc gauge (centre)
    arc_aqi = lv_arc_create(scr_dashboard);
    lv_obj_set_size(arc_aqi, 220, 220);
    lv_arc_set_range(arc_aqi, 1, 5);
    lv_arc_set_value(arc_aqi, 1);
    lv_arc_set_bg_angles(arc_aqi, 135, 405);
    lv_obj_set_style_arc_width(arc_aqi, 18, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_aqi, 18, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_aqi, lv_color_make(0x2C, 0x2C, 0x2A), LV_PART_MAIN);
    lv_obj_remove_style(arc_aqi, nullptr, LV_PART_KNOB);
    lv_obj_align(arc_aqi, LV_ALIGN_CENTER, 0, -30);
    lv_obj_clear_flag(arc_aqi, LV_OBJ_FLAG_CLICKABLE);  // read-only

    lbl_aqi_val = lv_label_create(scr_dashboard);
    lv_label_set_text(lbl_aqi_val, "--");
    lv_obj_set_style_text_font(lbl_aqi_val, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_aqi_val, lv_color_white(), 0);
    lv_obj_align_to(lbl_aqi_val, arc_aqi, LV_ALIGN_CENTER, 0, -10);

    lbl_aqi_text = lv_label_create(scr_dashboard);
    lv_label_set_text(lbl_aqi_text, "AQI");
    lv_obj_set_style_text_font(lbl_aqi_text, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_aqi_text, lv_color_make(0x88, 0x87, 0x80), 0);
    lv_obj_align_to(lbl_aqi_text, arc_aqi, LV_ALIGN_CENTER, 0, 22);

    // Metric tiles row (bottom half)
    struct { lv_obj_t** lbl; const char* title; lv_color_t col; } tiles[] = {
        { &lbl_eco2, "eCO\u2082 (ppm)",  lv_color_make(0x53, 0x4A, 0xB7) },
        { &lbl_tvoc, "TVOC (ppb)",        lv_color_make(0x0F, 0x6E, 0x56) },
        { &lbl_temp, "Temp (\u00b0C)",    lv_color_make(0x99, 0x3C, 0x1D) },
        { &lbl_hum,  "Humidity (%)",      lv_color_make(0x18, 0x5F, 0xA5) },
    };

    int tileW = (DISPLAY_WIDTH - 80) / 4;
    for (int i = 0; i < 4; i++) {
        lv_obj_t* tile = lv_obj_create(scr_dashboard);
        lv_obj_set_size(tile, tileW, 80);
        lv_obj_set_style_bg_color(tile, tiles[i].col, 0);
        lv_obj_set_style_bg_opa(tile, LV_OPA_30, 0);
        lv_obj_set_style_border_color(tile, tiles[i].col, 0);
        lv_obj_set_style_border_width(tile, 1, 0);
        lv_obj_set_style_radius(tile, 8, 0);
        lv_obj_set_style_pad_all(tile, 6, 0);
        lv_obj_align(tile, LV_ALIGN_BOTTOM_LEFT, 16 + i * (tileW + 12), -62);

        lv_obj_t* t = lv_label_create(tile);
        lv_label_set_text(t, tiles[i].title);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(t, lv_color_white(), 0);
        lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 2);

        *tiles[i].lbl = lv_label_create(tile);
        lv_label_set_text(*tiles[i].lbl, "--");
        lv_obj_set_style_text_font(*tiles[i].lbl, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(*tiles[i].lbl, lv_color_white(), 0);
        lv_obj_align(*tiles[i].lbl, LV_ALIGN_CENTER, 0, 8);
    }

    lbl_last_update = lv_label_create(scr_dashboard);
    lv_label_set_text(lbl_last_update, "No data yet");
    lv_obj_set_style_text_font(lbl_last_update, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_last_update, lv_color_make(0x5F, 0x5E, 0x5A), 0);
    lv_obj_align(lbl_last_update, LV_ALIGN_BOTTOM_LEFT, 16, -10);

    makeNavBar(scr_dashboard, 0);
}

// ---------------------------------------------------------------------------
// Build History screen (placeholder — add lv_chart for real history)
// ---------------------------------------------------------------------------
static void buildHistory() {
    scr_history = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_history, lv_color_make(0x1A, 0x1A, 0x18), 0);

    lv_obj_t* lbl = lv_label_create(scr_history);
    lv_label_set_text(lbl, "History — last 60 readings\n(lv_chart goes here)");
    lv_obj_set_style_text_color(lbl, lv_color_make(0xCE, 0xCB, 0xF6), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -30);

    makeNavBar(scr_history, 1);
}

// ---------------------------------------------------------------------------
// Build Settings screen (placeholder — add sliders for thresholds)
// ---------------------------------------------------------------------------
static void buildSettings() {
    scr_settings = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_settings, lv_color_make(0x1A, 0x1A, 0x18), 0);

    lv_obj_t* lbl = lv_label_create(scr_settings);
    lv_label_set_text(lbl, "Settings\n(threshold sliders go here)");
    lv_obj_set_style_text_color(lbl, lv_color_make(0xCE, 0xCB, 0xF6), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -30);

    makeNavBar(scr_settings, 2);
}

// ---------------------------------------------------------------------------
// Build Alerts screen
// ---------------------------------------------------------------------------
static void buildAlerts() {
    scr_alerts = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(scr_alerts, lv_color_make(0x1A, 0x1A, 0x18), 0);

    lv_obj_t* title = lv_label_create(scr_alerts);
    lv_label_set_text(title, LV_SYMBOL_WARNING " Alert log");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_make(0xEF, 0x9F, 0x27), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    alert_list = lv_list_create(scr_alerts);
    lv_obj_set_size(alert_list, DISPLAY_WIDTH - 32, DISPLAY_HEIGHT - 120);
    lv_obj_align(alert_list, LV_ALIGN_TOP_MID, 0, 48);
    lv_obj_set_style_bg_color(alert_list, lv_color_make(0x2C, 0x2C, 0x2A), 0);

    makeNavBar(scr_alerts, 3);
}

// ---------------------------------------------------------------------------
// Alert overlay (built once, hidden by default)
// ---------------------------------------------------------------------------
static void buildAlertOverlay() {
    // Overlay sits on the active screen — we re-parent it as needed.
    // Simpler: create it as a child of lv_layer_top() so it floats above all.
    alert_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(alert_overlay, DISPLAY_WIDTH, 60);
    lv_obj_align(alert_overlay, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(alert_overlay, lv_color_make(0xA3, 0x2D, 0x2D), 0);
    lv_obj_set_style_border_width(alert_overlay, 0, 0);
    lv_obj_set_style_radius(alert_overlay, 0, 0);
    lv_obj_add_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);

    lbl_alert_msg = lv_label_create(alert_overlay);
    lv_label_set_text(lbl_alert_msg, LV_SYMBOL_WARNING " Air quality alert!");
    lv_obj_set_style_text_font(lbl_alert_msg, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_alert_msg, lv_color_white(), 0);
    lv_obj_center(lbl_alert_msg);
}

// ---------------------------------------------------------------------------
void ui_init() {
    buildDashboard();
    buildHistory();
    buildSettings();
    buildAlerts();
    buildAlertOverlay();

    lv_scr_load(scr_dashboard);
    Serial.println("[UI] Screens created — Dashboard loaded");
}

// ---------------------------------------------------------------------------
void ui_updateTelemetry(const TelemetryData* data) {
    if (!data) return;

    // --- Dashboard AQI arc ---
    lv_arc_set_value(arc_aqi, data->aqi);
    lv_obj_set_style_arc_color(arc_aqi, aqiColor(data->aqi), LV_PART_INDICATOR);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d", data->aqi);
    lv_label_set_text(lbl_aqi_val, buf);
    lv_label_set_text(lbl_aqi_text, data->aqiLabel());

    // --- Metric tiles ---
    snprintf(buf, sizeof(buf), "%d", data->eco2_ppm);
    lv_label_set_text(lbl_eco2, buf);

    snprintf(buf, sizeof(buf), "%d", data->tvoc_ppb);
    lv_label_set_text(lbl_tvoc, buf);

    snprintf(buf, sizeof(buf), "%.1f", data->temperature);
    lv_label_set_text(lbl_temp, buf);

    snprintf(buf, sizeof(buf), "%.1f", data->humidity);
    lv_label_set_text(lbl_hum, buf);

    // --- Status + timestamp ---
    lv_label_set_text(lbl_status, LV_SYMBOL_WIFI " Live");
    snprintf(buf, sizeof(buf), "Updated %lus ago", data->timestamp_ms / 1000);
    lv_label_set_text(lbl_last_update, buf);

    // --- Alert overlay ---
    if (data->hasAlert()) {
        const char* alertType = data->alert_eco2 ? "High eCO\u2082"
                              : data->alert_tvoc  ? "High TVOC"
                              : data->alert_temp  ? "High temperature"
                              :                     "High humidity";
        snprintf(buf, sizeof(buf), LV_SYMBOL_WARNING " Alert: %s", alertType);
        lv_label_set_text(lbl_alert_msg, buf);
        lv_obj_clear_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);

        // Log to alerts screen
        lv_list_add_text(alert_list, buf);
    } else {
        lv_obj_add_flag(alert_overlay, LV_OBJ_FLAG_HIDDEN);
    }
}
