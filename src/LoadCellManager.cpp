#include "LoadCellManager.h"

/**
 * @brief Construct a new Load Cell Manager:: Load Cell Manager object
 * 
 */
LoadCellManager::LoadCellManager() {
    
}

/**
 * @brief init the ADC object and setup all, do a calibration
 * setup the differencial read between 0 and 1 channel
 * setup the gain to 64 to get a range scale from 0 to 1,28v
 * setup the conversionFactor to read directly milli Newton
 * 
 * @return true if init is successfull
 * @return false if init failed
 */
bool LoadCellManager::begin() {
    adc.begin(ADS1256_DRATE_3750SPS, ADS1256_GAIN_64, true);

    // compute the conversion factor for the load_cell
    // for a 200kg loadcell 2mv/V power at 10V, 
    //      max_voltage at full load is 10v * 0.002mv.v-1 = 0.02v
    //      fc = 1000(fo milli) * 9.81(for Newton) * 200kg /  0.02v = 10000kg/v
    const float cf = 1000 * 9.81 * LC_LOAD / (LC_VREF * LC_MVV);
    //const float cf = 1.0;
    log_v("convertion factor : %.5f", cf);
    adc.setConversionFactor(cf);

    adc.setChannel(0,1);

    int i=0;
    bool isReady = adc.isDRDY();
    while (!isReady && (i<10))
    {
        delay(100);
        i++;
        isReady = adc.isDRDY();
    }
    if (!isReady) return false;
    log_v("setchannel successfull");

    adc.sendCommand(ADS1256_CMD_SELFOCAL);
    isReady = adc.isDRDY();
    while (!isReady && (i<10))
    {
        delay(100);
        i++;
        isReady = adc.isDRDY();
    }
    log_v("calibration is ok");
    return isReady;
}


/**
 * @brief read the driver, call it @ 35khz
 * 
 */
void LoadCellManager::loop() {

    // Read the actual force and offset it
    const float load_force_mN = adc.readCurrentChannel() - calibration;
    
    // Filter the offset signal with the low_pass_filter
    load_avg  =  input_lpf.process(load_force_mN);
    //load_avg.addValue(loadforce_mN); // voltage is between AIN0 and AIN1
    if (sample_is_start) {
        if (!load_avg_sample.bufferIsFull()) {
            load_avg_sample.addValue(load_avg);
        } else {
            sample_is_start = false;
        }
    }
}

float LoadCellManager::calibrationOffset() {
    RunningAverage calib_avg(CALIB_SAMPLE);
    for (int i=0; i < CALIB_SAMPLE;  i++)  {
        const float adcvoltage = adc.readCurrentChannel();
        calib_avg.addValue(adcvoltage);
        delay(CALIB_MS);
    }
    calibration = calib_avg.getFastAverage();
    return calibration;
}


/**
 * @brief return the load read average on 3 sample @ 35khz, so around 1khz
 * 
 * @return uint32_t the load in mN (milli Newton)
 */
uint32_t LoadCellManager::getLoad() {
    const float load = load_avg;
    if (load <= 0.0) { return 0; }
    else if ( load > UINT32_MAX ) { return UINT32_MAX; }
    return (uint32_t) load;
}

/**
 * @brief start to read 500 sample to get data quality
 * sample record are done in loop.
 */
void LoadCellManager::startSample() {
    load_avg_sample.clear();
    sample_is_start = true;
}

String LoadCellManager::getStats() {
    char buffer[STAT_BUFFER_SIZE];
    int j;
    j = snprintf(buffer, STAT_BUFFER_SIZE, "CAL=%.5f:MIN=%.5f:MAX=%.5f:AVG=%.5f:STDD=%.5f:STDE=%.5f", 
        calibration,
        load_avg_sample.getMin(),
        load_avg_sample.getMax(),
        load_avg_sample.getFastAverage(),
        load_avg_sample.getStandardDeviation(),
        load_avg_sample.getStandardError());
    return String(buffer, j);
}

/**
 * @brief rebuild a new filter for the input signal
 * 
 * @param fc freq to cut, default is 30
 * @param q coeff, default is 0.5
 */
void LoadCellManager::setFilterParam(float fc, float q) {
    if ((fc>0 && fc<=  FREQ_SAMPLE) && (q>0 && q<1.5))  {
        input_lpf.~Biquad();
        input_lpf = Biquad(bq_type_lowpass, FREQ_SAMPLE / fc, q, 0);
    }
}



