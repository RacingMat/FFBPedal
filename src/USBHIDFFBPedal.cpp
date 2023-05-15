// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "USBHID.h"

#if CONFIG_TINYUSB_HID_ENABLED

#include "USBHIDFFBPedal.h"

USBHIDFFBPedal::USBHIDFFBPedal(): hid(), _x(0), _y(0){
    static bool initialized = false;
    if(!initialized){
        initialized = true;
        hid.addDevice(this, sizeof(report_descriptor));
    }
}

uint16_t USBHIDFFBPedal::_onGetDescriptor(uint8_t* dst){
    memcpy(dst, report_descriptor, sizeof(report_descriptor));
    return sizeof(report_descriptor);
}

void USBHIDFFBPedal::begin(){
    hid.begin();
}

void USBHIDFFBPedal::end(){

}

bool USBHIDFFBPedal::write(){
    hid_activepedal_report_t report = {
        .x       = _x,
        .y       = _y
    };
    return hid.SendReport(HID_REPORT_ID_GAMEPAD, &report, sizeof(report));
}

bool USBHIDFFBPedal::pedalMove(int16_t x, int16_t y){
    _x = x;
    _y = y;
    return true;
}

bool USBHIDFFBPedal::send(){
    return write();
}


#endif /* CONFIG_TINYUSB_HID_ENABLED */
