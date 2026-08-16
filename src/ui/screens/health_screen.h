// ============================================================================
// Suraksha+ Watch OS — Health Screen
// ============================================================================
#ifndef HEALTH_SCREEN_H
#define HEALTH_SCREEN_H
#include <lvgl.h>
namespace HealthScreen {
    lv_obj_t* create();
    void update(lv_obj_t* screen);
    void destroy(lv_obj_t* screen);
}
#endif
