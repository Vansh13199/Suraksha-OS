// ============================================================================
// Suraksha+ Watch OS — UI Theme
// Color palette, styles, and visual constants
// ============================================================================
#ifndef THEME_H
#define THEME_H

#include <lvgl.h>

namespace Theme {

    // --- Color Palette ---
    static const lv_color_t COLOR_PRIMARY       = lv_color_hex(0x1A73E8);  // Suraksha blue
    static const lv_color_t COLOR_DANGER        = lv_color_hex(0xE53935);  // SOS red
    static const lv_color_t COLOR_DANGER_DARK   = lv_color_hex(0xB71C1C);  // Dark red
    static const lv_color_t COLOR_SAFETY        = lv_color_hex(0x43A047);  // Green
    static const lv_color_t COLOR_ACTIVITY      = lv_color_hex(0xFB8C00);  // Orange
    static const lv_color_t COLOR_WARNING       = lv_color_hex(0xFFB300);  // Amber

    static const lv_color_t COLOR_BG            = lv_color_hex(0x0D1117);  // Dark background
    static const lv_color_t COLOR_SURFACE       = lv_color_hex(0x161B22);  // Card surface
    static const lv_color_t COLOR_SURFACE_LIGHT = lv_color_hex(0x21262D);  // Lighter surface

    static const lv_color_t COLOR_TEXT          = lv_color_hex(0xFFFFFF);  // Primary text
    static const lv_color_t COLOR_TEXT_DIM      = lv_color_hex(0x8B949E);  // Secondary text
    static const lv_color_t COLOR_TEXT_MUTED    = lv_color_hex(0x484F58);  // Muted text

    static const lv_color_t COLOR_HEART         = lv_color_hex(0xEF5350);  // Heart rate
    static const lv_color_t COLOR_BLE           = lv_color_hex(0x42A5F5);  // Bluetooth blue
    static const lv_color_t COLOR_BATTERY       = lv_color_hex(0x66BB6A);  // Battery green
    static const lv_color_t COLOR_BATTERY_LOW   = lv_color_hex(0xFF7043);  // Battery low

    // --- Font Sizes ---
    static const lv_font_t* FONT_SMALL   = &lv_font_montserrat_12;
    static const lv_font_t* FONT_NORMAL  = &lv_font_montserrat_14;
    static const lv_font_t* FONT_MEDIUM  = &lv_font_montserrat_16;
    static const lv_font_t* FONT_LARGE   = &lv_font_montserrat_20;
    static const lv_font_t* FONT_XLARGE  = &lv_font_montserrat_24;
    static const lv_font_t* FONT_TITLE   = &lv_font_montserrat_32;
    static const lv_font_t* FONT_TIME    = &lv_font_montserrat_48;

    // --- Dimensions ---
    static const int PADDING       = 8;
    static const int PADDING_LARGE = 16;
    static const int RADIUS        = 12;
    static const int RADIUS_LARGE  = 20;
    static const int ICON_SIZE     = 20;
    static const int STATUS_BAR_H  = 28;

    // --- Apply theme to LVGL ---
    void apply();
    void applyDarkTheme(lv_disp_t* disp);

    // --- Style helpers ---
    void styleCard(lv_obj_t* obj);
    void stylePrimaryButton(lv_obj_t* obj);
    void styleDangerButton(lv_obj_t* obj);
    void styleStatusBar(lv_obj_t* obj);

} // namespace Theme

#endif // THEME_H
