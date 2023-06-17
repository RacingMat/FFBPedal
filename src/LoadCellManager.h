/**
 * Copyright (c) Vincent Manoukian 2023.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE.md file in the root directory of this source tree.
 */

#ifndef _LOADCELL_MANAGER_H_
#define _LOADCELL_MANAGER_H_

#include <Arduino.h>
#include <RunningAverage.h>
#include <Biquad.h>

#include <ADS1256.h>
#include <SPI.h>

// Pin for stepper on ESP32
#define DIR_PIN     18
#define ENABLE_PIN  26
#define STEP_PIN    17

#define ADC_CLKSPD  7.68
#define ADC_VREF    2.5
#define GAIN        64

#define LC_VREF     5
#define LC_MVV      0.002
#define LC_LOAD     3

#define CALIB_SAMPLE    100
#define CALIB_MS        1

#define NB_SAMPLE           500
#define STAT_BUFFER_SIZE    200

#define FREQ_SAMPLE         4000.0
#define FC_CUT              30.0
#define Q_CUT               0.6

class LoadCellManager {

private:
    ADS1256 adc = ADS1256(ADC_CLKSPD, ADC_VREF, false);
    //RunningAverage load_avg = RunningAverage(3);
    float load_avg;
    Biquad input_lpf = Biquad(bq_type_lowpass, FREQ_SAMPLE / FC_CUT, Q_CUT, 0);
    float calibration = 0.0;

    RunningAverage load_avg_sample = RunningAverage(NB_SAMPLE);
    bool sample_is_start = false;
public:
    // init and calibration
    LoadCellManager();
    bool begin();
    float calibrationOffset();

    // read loadcell process
    void loop();
    uint32_t getLoad();

    // quality signal
    void startSample();
    String getStats();
    void setFilterParam(float fc, float q);

};

#endif