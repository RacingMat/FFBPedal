#include <Arduino.h>

#include <esp_log.h>

#include "USBHIDFFBPedal.h"
#include "USB.h"

USBHIDFFBPedal ffbPedal;

void setup() {
  Serial.begin(115200);
  log_i("Start the FFB Pedal");

  USB.manufacturerName("Vincent Manoukian");
  USB.productName("FFB Pedal");
  USB.firmwareVersion(1);
  USB.begin();

  ffbPedal.begin();
}

int16_t pos=0;

void loop() {

  pos++;
  ffbPedal.pedalMove(pos,0);

  ffbPedal.send();

}