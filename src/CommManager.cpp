/**
 * Copyright (c) Vincent Manoukian 2023.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE.md file in the root directory of this source tree.
 */

#include "CommManager.h"

CommManager::CommManager() {

}

/**
 * @brief init the object with the manager in main class to manage communication.
 * 
 * @param _vendor HID Vendor
 * @param _loadCell loadCell manager
 * @param _pedalEngine pedalEngine manager
 */
void CommManager::begin(USBHIDVendor *_vendor, LoadCellManager *_loadCell, PedalEngine *_pedalEngine) {
  vendor = _vendor;
  loadCell = _loadCell;
  pedalEngine = _pedalEngine;
}

uint8_t CommManager::process_mesg_0(uint16_t length_incoming,  uint8_t *buff_in, uint8_t *buff_output){
  const uint32_t position = pedalEngine->getPosition();
  const int32_t speed = pedalEngine->getSpeed();
  const uint16_t motor_command = pedalEngine->getMotorCommand();
  const uint32_t load = loadCell->getLoad();
  uint8_t size;
  memcpy(buff_output + size, &position, (size+=sizeof(uint32_t)) );
  memcpy(buff_output + size, &speed,    (size+=sizeof(int32_t)) );
  memcpy(buff_output + size, &motor_command,  (size+=sizeof(uint16_t)) );
  memcpy(buff_output + size, &load,     (size+=sizeof(uint32_t)) );
  return size;
}

/**
 * @brief Process the incoming data from HID Vendor
 * 
 * @param length_incoming How many byte  to read
 * @param buff_output The buffer used for the response
 * @return uint8_t the response size
 */
uint8_t CommManager::process_incoming(uint16_t length_incoming, uint8_t *buff_output){
  // get the incoming message
  if (vendor == NULL) {
    log_e("VENDOR not init");
    return 0;
  }
  uint8_t incoming[64] = {0};
  const size_t len_read = vendor->readBytes(incoming, (size_t)length_incoming);
  if (len_read != length_incoming) {
    log_e("Can't read all data from HID %lu/%lu", length_incoming, len_read);
    return 0;
  }

  const uint8_t cmd = incoming[0];
  uint8_t response_size = 0;
  switch (cmd)
  {
  case 0: // get Info
    response_size = process_mesg_0(length_incoming-1, incoming + 1, buff_output);
    break;
  //TODO implement all
  
  default:
    break;
  }

  return response_size;
}


