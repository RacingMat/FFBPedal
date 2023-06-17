/**
 * Copyright (c) Vincent Manoukian 2023.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE.md file in the root directory of this source tree.
 */

#ifndef _PEDAL_ENGINE_H_
#define _PEDAL_ENGINE_H_

#include <Arduino.h>
#include <RunningAverage.h>

// Pin for stepper on ESP32
#define HOME_PIN_ENDSTOP 8
#define END_PIN_ENDSTOP  9

enum PedalEngine_State_e {
    PE_UNKOWN,
    PE_ONERROR,
    PE_INIT,
    PE_CALIBRATED,
    PE_OK
};

class PedalEngine {
private:
    // motor settings
    uint16_t mm_turn = 10;             // SFU 1610 
    uint16_t speed_max = 3000;         // max speed in rpm
    uint16_t accel_max = 10000;        // max accel in rpm2
    uint16_t current_max = 5000;       // max current mAh
    uint16_t motor_command = 0;         // the motor command to apply 
    uint16_t motor_command_range = 1000; // the motor command range


    // Movement engine 
    int32_t position = 0;             // stepper position in step
    //int32_t position_target_s = 0;      // stepper target for next move in step
    uint32_t force = 0;                 // the force applyed on the pedal
    uint32_t force_effects = 0;         // the counter force coming from local effect
    int32_t speed = 0;                  // the actual speed in rpm
    PedalEngine_State_e state = PE_UNKOWN; // init the state of Pedal_Engine to unknow

    // effect settings
    uint32_t damper_cst = 1000;         // Cste for damping
    uint32_t friction_cst = 200;        // Cste for friction
    uint8_t damper_awd = 0;             // range [0-200] -> [0-1.0]
    uint8_t damper_rwd = 0;             // range [0-200] -> [0-1.0]
    uint8_t friction_awd = 0;           // range [0-200] -> [0-1.0]
    uint8_t friction_rwd = 0;           // range [0-200] -> [0-1.0]

    // mechanical settings
    const float endstop_max_mm = 90.0;  // endstop in mm
    const float endstop_min_mm = 0.0; // endstop in mm

    // pedal settings
    float position_min_mm = 10;       // user min position
    float position_max_mm = 60;       // user max position
    uint32_t force_min  = 0;          // mN    
    uint32_t force_max  = 30000;      // mN

    // setup
    // add specific init heve, for exemple the driver init

    // effect computing
    uint32_t computeDamperFriction();
    uint32_t clipping(uint32_t force);


public:
    // setup
    PedalEngine();
    bool begin();

    // effect computing
    void setLoad(uint32_t incoming_force);
    void computeEffectAndMovement();

    // mouvment
    bool calibration();
    bool sendMotorCommand();

    // accessor
    float getPosition();
    float getPositionMin();
    float getPositionMax();
    float getSpeed();
    void setPositionMin(float new_pos);
    void setPositionMax(float new_pos);
    uint16_t getMotorCommand();
    
    // HID
    uint32_t getHIDValue();

};

#endif