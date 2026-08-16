// ============================================================================
// Suraksha+ Watch OS — Watch Face Screen
// Primary screen with time, date, status indicators
// ============================================================================
#ifndef WATCHFACE_SCREEN_H
#define WATCHFACE_SCREEN_H

#include <lvgl.h>

namespace WatchfaceScreen {
    lv_obj_t* create();
    void update(lv_obj_t* screen);
    void destroy(lv_obj_t* screen);
}

#endif // WATCHFACE_SCREEN_H
