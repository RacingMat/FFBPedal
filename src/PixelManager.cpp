#include "PixelManager.h"

void PixelManager::begin() {
    // Init the led  grd status
    #if defined(NEOPIXEL_POWER)
    // If this board has a power control pin, we must set it to output and high
    // in order to enable the NeoPixels. We put this in an #if defined so it can
    // be reused for other boards without compilation errors
        pinMode(NEOPIXEL_POWER, OUTPUT);
        digitalWrite(NEOPIXEL_POWER, HIGH);
    #endif

    pixel.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
    pixel.setBrightness(10);  // not so bright
    pixel.fill(PIXEL_STARTUP);
    pixel.show();
}

void PixelManager::changeColor(Pixel_Color_e new_color) {
    color = new_color;
    pixel.fill(color);
    pixel.show();
}
