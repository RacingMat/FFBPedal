/**
 * Copyright (c) Vincent Manoukian 2023.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE.md file in the root directory of this source tree.
 */

#include "USBHIDFFBPedal.h"

#if CONFIG_TINYUSB_HID_ENABLED


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
    hid.end();
}

bool USBHIDFFBPedal::ready() {
    return hid.ready();
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
    bool result = false;
    if ( hid.ready() ) {
        result = write();
    }
    return result;
}

#endif /* CONFIG_TINYUSB_HID_ENABLED */
