// ============================================================================
// Suraksha+ Watch OS — Display Driver Implementation
// ============================================================================
#include "display_driver.h"

DisplayDriver& DisplayDriver::getInstance() {
    static DisplayDriver instance;
    return instance;
}

void DisplayDriver::init() {
    Serial.println("[DISP] Initializing GC9A01 display...");

    // Initialize TFT
    _tft.begin();
    _tft.setRotation(0);       // 0° rotation for circular display
    _tft.fillScreen(TFT_BLACK);
    _tft.setSwapBytes(true);

    Serial.printf("[DISP] TFT initialized: %dx%d\n", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // Allocate LVGL draw buffers in PSRAM
    _buf1 = (lv_color_t*)ps_malloc(DRAW_BUF_SIZE * sizeof(lv_color_t));
    _buf2 = (lv_color_t*)ps_malloc(DRAW_BUF_SIZE * sizeof(lv_color_t));

    if (!_buf1 || !_buf2) {
        Serial.println("[DISP] ❌ PSRAM allocation failed! Falling back to SRAM...");
        if (_buf1) free(_buf1);
        if (_buf2) free(_buf2);
        // Smaller buffers in SRAM
        _buf1 = (lv_color_t*)malloc(DISPLAY_WIDTH * 10 * sizeof(lv_color_t));
        _buf2 = nullptr;
    }

    if (!_buf1) {
        Serial.println("[DISP] ❌ CRITICAL: Cannot allocate display buffer!");
        return;
    }

    // Initialize LVGL draw buffer
    if (_buf2) {
        lv_disp_draw_buf_init(&_drawBuf, _buf1, _buf2, DRAW_BUF_SIZE);
        Serial.println("[DISP] Double-buffered (PSRAM)");
    } else {
        lv_disp_draw_buf_init(&_drawBuf, _buf1, nullptr, DISPLAY_WIDTH * 10);
        Serial.println("[DISP] Single-buffered (SRAM fallback)");
    }

    // Initialize LVGL display driver
    lv_disp_drv_init(&_dispDrv);
    _dispDrv.hor_res = DISPLAY_WIDTH;
    _dispDrv.ver_res = DISPLAY_HEIGHT;
    _dispDrv.flush_cb = _lvglFlushCb;
    _dispDrv.draw_buf = &_drawBuf;
    _dispDrv.user_data = this;

    // Round display: set rounder callback if needed
    // _dispDrv.rounder_cb = nullptr;

    _lvDisp = lv_disp_drv_register(&_dispDrv);

    if (_lvDisp) {
        Serial.println("[DISP] ✅ LVGL display driver registered");
    } else {
        Serial.println("[DISP] ❌ LVGL display driver registration failed!");
    }
}

void DisplayDriver::_lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    DisplayDriver* self = (DisplayDriver*)disp->user_data;
    TFT_eSPI* tft = &self->_tft;

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft->startWrite();
    tft->setAddrWindow(area->x1, area->y1, w, h);
    tft->pushColors((uint16_t*)&color_p->full, w * h, true);
    tft->endWrite();

    lv_disp_flush_ready(disp);
}
