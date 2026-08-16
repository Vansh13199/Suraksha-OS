// ============================================================================
// Suraksha+ Watch OS — Apps Menu & Settings Screen Headers
// ============================================================================
#ifndef APPS_MENU_SCREEN_H
#define APPS_MENU_SCREEN_H
#include <lvgl.h>
namespace AppsMenuScreen {
    lv_obj_t* create();
    void update(lv_obj_t* screen);
}
#endif
