#ifndef _PEDAL_ENGINE_H_
#define _PEDAL_ENGINE_H_

#include <Arduino.h>
#include <FastAccelStepper.h>
#include <TMCStepper.h>

#include <RunningAverage.h>

// Pin for stepper on ESP32
#define TMC_DIR_PIN     42
#define TMC_ENABLE_PIN  41
#define TMC_STEP_PIN    40
#define TMC_CS_PIN      6
#define TMC_R_SENSE     0.075f

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
    uint16_t mstep = 8 ;               // 8 mstep on TMC
    uint16_t mstep_turn = mstep * 200; // mstep * 200 for 1.9° stepper
    uint16_t mm_turn = 10;             // SFU 1610 
    uint16_t speed_max_spt = 20000;    // max speed in Step/t
    uint16_t accel_max_spt2 = 6400;    // max accel in Step/t²
    uint16_t current_max = 400;        // max current mAh

    // Movement engine 
    int32_t position_s = 0;             // stepper position in step
    int32_t position_target_s = 0;      // stepper target for next move in step
    uint32_t force = 0;                 // the force applyed on the pedal
    uint32_t force_effects = 0;         // the counter force coming from local effect
    int32_t speed = 0;                  // the actual speed in step/s
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
    const float endstop_min_mm = -90.0; // endstop in mm
    int32_t endstop_max_s = endstop_max_mm * mstep_turn / mm_turn;   // endstop max position in step
    int32_t endstop_min_s = endstop_min_mm * mstep_turn / mm_turn;   // endstop min position in step

    // pedal settings
    float position_min_mm = -30;
    float position_max_mm = 30;
    int32_t position_min_s = position_min_mm * mstep_turn / mm_turn; // -4800 step
    int32_t position_max_s = position_max_mm * mstep_turn / mm_turn; // 4800 step
    uint32_t force_min  = 0;          // mN    
    uint32_t force_max  = 30000;      // mN

    // Stepper
    TMC5160Stepper driver = TMC5160Stepper(TMC_CS_PIN, TMC_R_SENSE,37,39,38);

    // setup
    bool PedalEngine::setupRamp();

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
    float getPosition_target();
    float getPositionMin();
    float getPositionMax();
    float getSpeed();
    void setPositionMin(float new_pos);
    void setPositionMax(float new_pos);
    uint16_t getCurrent();
    
    // HID
    uint32_t getHIDValue();

};

#endif