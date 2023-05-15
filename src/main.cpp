#include <Arduino.h>

#include <SerialCommands.h>

#include "PedalEngine.h"

#include "USBHIDFFBPedal.h"
#include "USB.h"

// Pedal engine
PedalEngine pedalEngine;

// HID Object
USBHIDFFBPedal ffbPedal;

// UART communication object
char serial_command_buffer_[32];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", ":");
void cmd_unrecognized(SerialCommands* sender, const char* cmd);
void cmd_get_info(SerialCommands* sender);
void cmd_effect_abs(SerialCommands* sender);
SerialCommand cmd_get_info_   ("INFO", cmd_get_info);
SerialCommand cmd_effect_abs_ ("ABS", cmd_effect_abs);

void setup() {
  Serial.begin(115200);

  serial_commands_.SetDefaultHandler(cmd_unrecognized);
  serial_commands_.AddCommand(&cmd_get_info_);
  serial_commands_.AddCommand(&cmd_effect_abs_);

  USB.manufacturerName("Vincent Manoukian");
  USB.productName("FFB Pedal");
  USB.firmwareVersion(1);
  USB.begin();

  ffbPedal.begin();
}

int16_t pos=0;

void loop() {

  serial_commands_.ReadSerial();

  pos++;
  ffbPedal.pedalMove(pos,0);

  ffbPedal.send();

}

// *************************************** UART COMMAND ******************************************
/**
 * @brief return all FFB Pedal info
 * 
 * @param sender the serial object
 */
void cmd_get_info(SerialCommands* sender)
{
  sender->GetSerial()->printf("R:INFO:pos=%lu,min=%lu,max=%lu\n", 
                              pedalEngine.getPosition(),
                              pedalEngine.getMinPosition(),
                              pedalEngine.getMaxPosition());
}

/**
 * @brief incoming abs effect request.
 * First param is the state (0/1)
 * Second param is the optional duration in ms
 * 
 * @param sender the serial object
 */
void cmd_effect_abs(SerialCommands* sender)
{
  bool success = true;
  bool state = 0;
  int duration_ms = -1;

  char* state_str = sender->Next();
  if ((state_str == NULL) || (state_str[0] != '0' && state_str[0] != '1')) {
    success = false;
    sender->GetSerial()->printf("E:ABS:Wrong param\n");
    return;
  } else {
    state = atoi(state_str);
  }

  char* duration_str = sender->Next();
	if (duration_str != NULL)
	{
		duration_ms = atoi(duration_str);
    // if the response is 0
    if (duration_ms == 0) {
      success = false;
    }
	}

  //TODO manage the abs asking effect.

  sender->GetSerial()->printf("R:ABS:%d:%d\n", state, duration_ms);
}

/**
 * @brief handler to answer to an unknow command
 * 
 * @param sender 
 * @param cmd 
 */
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
  sender->GetSerial()->print("E:Unrecognized command [");
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
}
