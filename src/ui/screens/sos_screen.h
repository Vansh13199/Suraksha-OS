// ============================================================================
// Suraksha+ Watch OS — SOS Active Screen
// Full-screen emergency alert with cancel capability
// ============================================================================
#ifndef SOS_SCREEN_H
#define SOS_SCREEN_H

#include <lvgl.h>

namespace SOSScreen {
    lv_obj_t* create();
    void update(lv_obj_t* screen);
    void destroy(lv_obj_t* screen);
}

#endif // SOS_SCREEN_H
