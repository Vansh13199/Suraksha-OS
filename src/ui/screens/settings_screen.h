// ============================================================================
// Suraksha+ Watch OS — Settings Screen
// ============================================================================
#ifndef SETTINGS_SCREEN_H
#define SETTINGS_SCREEN_H
#include <lvgl.h>
namespace SettingsScreen {
    lv_obj_t* create();
    void update(lv_obj_t* screen);
}
#endif
