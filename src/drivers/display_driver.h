// ============================================================================
// Suraksha+ Watch OS — Display Driver
// GC9A01 240x240 Round TFT via TFT_eSPI + LVGL Integration
// ============================================================================
#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "../config/hardware_config.h"

class DisplayDriver {
public:
    static DisplayDriver& getInstance();

    void init();
    TFT_eSPI* getTft() { return &_tft; }

    // LVGL display driver
    lv_disp_t* getLvDisplay() { return _lvDisp; }

private:
    DisplayDriver() = default;

    // LVGL flush callback (static for C API)
    static void _lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);

    TFT_eSPI _tft;
    lv_disp_t* _lvDisp = nullptr;
    lv_disp_draw_buf_t _drawBuf;
    lv_disp_drv_t _dispDrv;

    // Double-buffered draw buffers (allocated in PSRAM)
    lv_color_t* _buf1 = nullptr;
    lv_color_t* _buf2 = nullptr;

    static const uint32_t DRAW_BUF_SIZE = DISPLAY_WIDTH * 40; // 40 lines
};

#endif // DISPLAY_DRIVER_H
