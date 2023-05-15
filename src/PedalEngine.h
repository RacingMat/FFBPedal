#ifndef _PEDAL_ENGINE_H_
#define _PEDAL_ENGINE_H_

#include <Arduino.h>

class PedalEngine {
private:
    unsigned long position = 0;
    unsigned long min_position = 0;
    unsigned long max_position = 1000;

public:
    PedalEngine();
    unsigned long getPosition();
    unsigned long getMinPosition();
    unsigned long getMaxPosition();
};

#endif