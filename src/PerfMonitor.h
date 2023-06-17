#ifndef _PERF_MONITOR_H
#define _PERF_MONITOR_H

#include <Arduino.h>

#include "esp_freertos_hooks.h"

// for 240Mhz CPU
//#define  MaxIdleCalls 1900000.0
#define  MaxIdleCalls 7488000.0

/**
 * @brief Class based on the work of https://github.com/Carbon225/esp32-perfmon
 * Incompatible with esp32S3 so implemented here
 * 
 */
class PerfMonitor {
    private :
        static int32_t idle0Calls;
        static int32_t idle1Calls;
        static bool idle_task_0();
        static bool idle_task_1();
        static void perfmon_task(void *args);
        TaskHandle_t perfmon_task_handle = NULL;
    public  :
        static float cpu0;
        static float cpu1;
        bool start();
        bool stop();

};

#endif