#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "office_state.h"

// Rendering port of the upstream pixel scene to our existing Adafruit driver.
// One framebuffer, allocated only at startup. No PSRAM or new library required.
class DemoRenderer {
public:
    bool begin(Adafruit_ST7789& display);
    void render(OfficeState& office);
    void screenshot(Stream& stream);
private:
    Adafruit_ST7789* _display = nullptr;
    GFXcanvas16 _canvas{SCREEN_W, SCREEN_H};
    void sprite(int x, int y, const uint16_t* data, int w, int h, bool flip = false);
    void furniture();
    void character(const Character& ch);
    void labels(const Character* chars);
};
