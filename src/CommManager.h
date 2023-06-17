/**
 * Copyright (c) Vincent Manoukian 2023.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE.md file in the root directory of this source tree.
 */

#ifndef _COMM_MANAGER_H_
#define _COMM_MANAGER_H_

#include <Arduino.h>
#include "USBHIDVendor.h"
#include "PedalEngine.h"
#include "LoadCellManager.h"

class CommManager{

    private:
        // UART command
        /*void cmd_unrecognized();
        void cmd_get_info();
        void cmd_effect_abs();
        void cmd_loadcell_start_sample();
        void cmd_loadcell_stat();
        void cmd_loadcell_filter();*/
        USBHIDVendor *vendor = NULL;
        LoadCellManager *loadCell = NULL;
        PedalEngine *pedalEngine = NULL;

        uint8_t process_mesg_0(uint16_t length_incoming, uint8_t *buff_in, uint8_t *buff_output);


    public:
        CommManager();
        void begin(USBHIDVendor *_vendor, LoadCellManager *_loadCell, PedalEngine *_pedalEngine);

        uint8_t process_incoming(uint16_t length_incoming, uint8_t *buff_output);
};

#endif