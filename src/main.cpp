#include <Arduino.h>

#include "PixelManager.h"

#include "PerfMonitor.h"

#include "PedalEngine.h"

#include "LoadCellManager.h"

#include "USBHIDVendor.h"
#include "USBHIDFFBPedal.h"
#include "USB.h"

#include "CommManager.h"

// ATTENTION : overide by the ADC.begin
// load arduino esp32 default
#define SPI_CLK   12
#define SPI_MISO  13
#define SPI_MOSI  11
#define SPI_CS    10

// The Pixel Manager
PixelManager pixelManager;

// ESP32 performance monitor
PerfMonitor perfMonitor;

// Pedal engine
PedalEngine pedalEngine;

// LoadCell
LoadCellManager loadCell;

// USB Object
USBHIDFFBPedal ffbPedal;
USBHIDVendor vendor;
bool previousHIDstate=false;

// Communication manager
CommManager commManager;

// Timer for periodic operation (Sensor,FFB, etc.)
hw_timer_t *Timer0_Cfg = NULL;                      // Timer_0 for FFB is design to operate @ 1000hz (Engine + FFB)
hw_timer_t *Timer1_Cfg = NULL;                      // Timer_1 for Sensor are design to operate @ 4000hz (LoadCell + Position Sensor)
volatile SemaphoreHandle_t timerSemaphore_ffb;      // semaphore to send 'time to compute FFB' to loop()
volatile SemaphoreHandle_t timerSemaphore_sensor;   // semaphore to send 'time to check SENSOR' to loop()

static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
  if(event_base == ARDUINO_USB_EVENTS){
    arduino_usb_event_data_t * data = (arduino_usb_event_data_t*)event_data;
    switch (event_id){
      case ARDUINO_USB_STARTED_EVENT:
        log_d("USB PLUGGED");
        break;
      case ARDUINO_USB_STOPPED_EVENT:
        log_d("USB UNPLUGGED");
        break;
      case ARDUINO_USB_SUSPEND_EVENT:
        log_d("USB SUSPENDED: remote_wakeup_en: %u", data->suspend.remote_wakeup_en);
        break;
      case ARDUINO_USB_RESUME_EVENT:
        log_d("USB RESUMED");
        break;
      
      default:
        break;
    } 
  } else if(event_base == ARDUINO_USB_HID_VENDOR_EVENTS){
    arduino_usb_hid_vendor_event_data_t * data = (arduino_usb_hid_vendor_event_data_t*)event_data;
    switch (event_id){
      case ARDUINO_USB_HID_VENDOR_GET_FEATURE_EVENT:
        log_v("HID VENDOR GET FEATURE: len:%u", data->len);
        log_v("%s",String(data->buffer, data->len).c_str());
        break;
      case ARDUINO_USB_HID_VENDOR_SET_FEATURE_EVENT:
        log_v("HID VENDOR SET FEATURE: len:%u", data->len);
        log_v("%s",String(data->buffer, data->len).c_str());
        break;
      case ARDUINO_USB_HID_VENDOR_OUTPUT_EVENT:
        log_v("HID VENDOR OUTPUT: len:%u", data->len);
        {
          uint8_t buffer[64];
          uint8_t size_response = commManager.process_incoming(data->len, buffer);
          if (size_response > 0) {
            vendor.write(buffer, size_response);
          }
        }
        break;
      
      default:
        break;
    }
  }
}

void infos()
{
  esp_chip_info_t out_info;
  esp_chip_info(&out_info);
  log_v("CPU freq : %u Mhz",ESP.getCpuFreqMHz());
  log_v("CPU cores : %d", out_info.cores);
  log_v("Flash size : %d MB", ESP.getFlashChipSize() / 1000000);
  log_v("Free RAM :  %u bytes",ESP.getFreeHeap());
  log_v("tskIDLE_PRIORITY : %l",(long)tskIDLE_PRIORITY);
  log_v("configMAX_PRIORITIES : %l",(long)configMAX_PRIORITIES);
  log_v("configTICK_RATE_HZ : %d",configTICK_RATE_HZ);
}

void IRAM_ATTR Timer0_FFB_ISR() {
  xSemaphoreGiveFromISR(timerSemaphore_ffb, NULL);
}

void IRAM_ATTR Timer1_Sensor_ISR()  {
  xSemaphoreGiveFromISR(timerSemaphore_sensor, NULL);
}

void send_hid_position(void *)  {
  while (1) {
    const bool send = ffbPedal.send();
    if (!send && previousHIDstate) {
      log_i("HID become not ready, can't send");
      pixelManager.changeColor(PIXEL_HID_ERROR);
      previousHIDstate = false;
    }
    if (send && !previousHIDstate) {
      log_i("HID become ready, send");
      pixelManager.changeColor(PIXEL_OK);
      previousHIDstate  = true;
    }
    vTaskDelay(pdMS_TO_TICKS( 1 ));
  }
}

void setup() {
  Serial.begin(115200);

  log_i("Start system - CPU Info");
  infos();

  // Init the pixel manager for visual feedback
  PixelManager pixelManager;

  // Init the USB Devices: CDC for virtual serial, and HID for ffb reports
  USB.manufacturerName("Vincent Manoukian");
  USB.productName("FFB Pedal");
  USB.firmwareVersion(3);
  USB.onEvent(usbEventCallback);
  vendor.onEvent(usbEventCallback);

  vendor.begin();
  ffbPedal.begin();
  USB.begin();
  log_i("USB Init done");

  // init the SPI hardware
  // TODO check if it's ok to increase with those hardware, srtange SPI Init process
  //SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI);
  //SPI.setFrequency(8000000); 
  const bool initloadcell = loadCell.begin();
  log_i("LoadCell init : %d", initloadcell);

  // init the pedal
  pedalEngine.begin();

  // init the commManager
  commManager.begin(&vendor, &loadCell, &pedalEngine);

  // Start to monitor performance
  perfMonitor.start();
  log_i("PerfMonitor started");

  // LoadCell Calibration
  loadCell.calibrationOffset();

  // Start timer to manage system
  Timer0_Cfg = timerBegin(2, 80, true);
  timerAttachInterrupt(Timer0_Cfg, &Timer0_FFB_ISR, false);
  timerAlarmWrite(Timer0_Cfg, 1000, true);
  timerSemaphore_ffb = xSemaphoreCreateBinary();

  Timer1_Cfg = timerBegin(3, 80, true);
  timerAttachInterrupt(Timer1_Cfg, &Timer1_Sensor_ISR, false);
  timerAlarmWrite(Timer1_Cfg, 250, true);
  timerSemaphore_sensor = xSemaphoreCreateBinary();

  timerAlarmEnable(Timer0_Cfg);
  timerAlarmEnable(Timer1_Cfg);

  //start the task render to HID
  xTaskCreatePinnedToCore(send_hid_position, "send hid", 3072, NULL, 2, NULL, 0);

  pedalEngine.calibration(); //TODO add a start process with error management

  log_i("setup finish");
  pixelManager.changeColor(PIXEL_OK);

}

int16_t pos=0;
int ffb_statistic, sensor_statistic;
uint64_t lastprint;

void loop() {

  // Time to read sensor @4khz
  if (xSemaphoreTake(timerSemaphore_sensor, 0) == pdTRUE){
    
    loadCell.loop();  // read the sensor

    const uint32_t pedal_load = loadCell.getLoad();   // get the pedal average load
    pedalEngine.setLoad(pedal_load);                  // compute the next position

    sensor_statistic++;
  }

  // Time to read FFB @1khz
  if (xSemaphoreTake(timerSemaphore_ffb, 0) == pdTRUE) {

    pedalEngine.computeEffectAndMovement();  // Compute the FFB and movement
    pedalEngine.sendMotorCommand();          // Move the pedal

    // Update the pedal position for the next HID report
    ffbPedal.pedalMove(pedalEngine.getHIDValue(),0); // Update the axis X with the position

    ffb_statistic++;
  }


  if (millis() -  lastprint >  100) {
    //log_v("nb processing ffb/sensor: %d - %d",ffb_statistic,sensor_statistic);
    log_d("FFB data : %6dmN -> %6d/%6d (pos/target en microstep)", 
      loadCell.getLoad(), pedalEngine.getPosition(), pedalEngine.getPosition_target());
    ffb_statistic=0;
    sensor_statistic=0;
    lastprint= millis();
  }

}