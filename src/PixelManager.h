#ifndef _PIXEL_MANAGER_H_
#define _PIXEL_MANAGER_H_

#include <Adafruit_NeoPixel.h>

#define NUMPIXELS 1
#define PIXEL_GPIO 48

enum Pixel_Color_e {
    PIXEL_STARTUP = 0x0000FF,
    PIXEL_OK = 0x00FF00,
    PIXEL_ERROR = 0xFF0000,
    PIXEL_HID_ERROR = 0xFFA500
};

class PixelManager {
private:
    Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIXEL_GPIO, NEO_GRB + NEO_KHZ800);
    Pixel_Color_e color;
public:
    void begin();
    void changeColor(Pixel_Color_e new_color);
};

#endif