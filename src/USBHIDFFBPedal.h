/**
 * Copyright (c) Vincent Manoukian 2023.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE.md file in the root directory of this source tree.
 */

#pragma once
#include "USBHID.h"

#if CONFIG_TINYUSB_HID_ENABLED

// ActivePedal Report Descriptor Template
// 2 joysticks with following layout
// | X | Y |
#define TUD_HID_REPORT_DESC_ACTIVEPEDAL(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    /* 8 bit X, Y, Z, Rz, Rx, Ry (min -127, max 127 ) */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_LOGICAL_MIN_N  ( 0x8000                                 , 2) ,\
    HID_LOGICAL_MAX_N  ( 0x7fff                                 , 2) ,\
    HID_REPORT_COUNT   ( 2                                      ) ,\
    HID_REPORT_SIZE    ( 16                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

/// HID Gamepad Protocol Report.
typedef struct TU_ATTR_PACKED
{
  int16_t  x;         ///< Delta x  movement of left analog-stick
  int16_t  y;         ///< Delta y  movement of left analog-stick
} hid_activepedal_report_t;

static const uint8_t report_descriptor[] = {
    TUD_HID_REPORT_DESC_ACTIVEPEDAL(HID_REPORT_ID(HID_REPORT_ID_GAMEPAD))
};

class USBHIDFFBPedal: public USBHIDDevice {
private:
    USBHID hid;
    int16_t  _x;         ///< Delta x  movement of left analog-stick
    int16_t  _y;         ///< Delta y  movement of left analog-stick
    bool write();
public:
    USBHIDFFBPedal(void);
    void begin(void);
    void end(void);
    bool send();

    bool pedalMove(int16_t x, int16_t y);

    bool ready();

    // internal use
    uint16_t _onGetDescriptor(uint8_t* buffer);
};

#endif
