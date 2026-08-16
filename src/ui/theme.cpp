// ============================================================================
// Suraksha+ Watch OS — UI Theme Implementation
// ============================================================================
#include "theme.h"

namespace Theme {

    void apply() {
        // Will be called after display init
    }

    void applyDarkTheme(lv_disp_t* disp) {
        lv_theme_t* theme = lv_theme_default_init(
            disp,
            COLOR_PRIMARY,
            COLOR_DANGER,
            true,                   // Dark mode
            FONT_MEDIUM
        );
        lv_disp_set_theme(disp, theme);
    }

    void styleCard(lv_obj_t* obj) {
        lv_obj_set_style_bg_color(obj, COLOR_SURFACE, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(obj, RADIUS, 0);
        lv_obj_set_style_pad_all(obj, PADDING, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
    }

    void stylePrimaryButton(lv_obj_t* obj) {
        lv_obj_set_style_bg_color(obj, COLOR_PRIMARY, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(obj, RADIUS_LARGE, 0);
        lv_obj_set_style_text_color(obj, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(obj, FONT_MEDIUM, 0);
        lv_obj_set_style_pad_ver(obj, 10, 0);
        lv_obj_set_style_pad_hor(obj, 20, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_set_style_shadow_width(obj, 0, 0);

        // Pressed state
        lv_obj_set_style_bg_color(obj, lv_color_darken(COLOR_PRIMARY, 30), LV_STATE_PRESSED);
    }

    void styleDangerButton(lv_obj_t* obj) {
        lv_obj_set_style_bg_color(obj, COLOR_DANGER, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(obj, RADIUS_LARGE, 0);
        lv_obj_set_style_text_color(obj, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(obj, FONT_MEDIUM, 0);
        lv_obj_set_style_pad_ver(obj, 10, 0);
        lv_obj_set_style_pad_hor(obj, 20, 0);
        lv_obj_set_style_border_width(obj, 0, 0);

        // Pressed state
        lv_obj_set_style_bg_color(obj, COLOR_DANGER_DARK, LV_STATE_PRESSED);
    }

    void styleStatusBar(lv_obj_t* obj) {
        lv_obj_set_style_bg_color(obj, COLOR_BG, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
        lv_obj_set_style_radius(obj, 0, 0);
        lv_obj_set_style_pad_all(obj, 4, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
    }

} // namespace Theme
