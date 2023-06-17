/**
 * Copyright (c) Vincent Manoukian 2023.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE.md file in the root directory of this source tree.
 */

#include "PedalEngine.h"

PedalEngine::PedalEngine() {

};

bool PedalEngine::begin() {
  bool result;
  log_v("Init the Pedal Engine");
  
  const bool isInitOK = true;

  state = isInitOK ? PE_INIT : PE_ONERROR;

  return isInitOK;
}


/**************************************** FFB PROCESSOR **************************************/

uint32_t PedalEngine::clipping(uint32_t f){
  // clipping
  if (f < force_min) f = force_min;
  if (f > force_max) f = force_max;
  return f;
}

/**
 * @brief set the new_load on pedal
 * call it each time a new load, freq is not important must be > 1khz
 * 
 * @param force apply on the pedals in mN
 */
void PedalEngine::setLoad(uint32_t incoming_force) {
  // store the actual force
  force = clipping(incoming_force);

}

/**
 * @brief this compute the effect and the next movement. Called it at 1khz.
 * This compute the next travel todo, compute the friction/damper effect.
 * 
 * @param force apply on the pedals in mN
 */
void PedalEngine::computeEffectAndMovement() {
  // Compute the effect modulation
  const uint32_t force_effect = computeDamperFriction();
  long new_force = force - force_effect;

  // clipping
  new_force = clipping(new_force);

  // implement linear move : min_force = pos_min, max_forces=pos_max
  const int32_t force_position = map(position, position_min_mm, position_max_mm, force_min, force_max);

  // Compute the motor command
  if (force<force_position) {
    log_v("actual force < position force => apply RW command");
    motor_command = -motor_command_range;
  }
  if (force>force_position) {
    log_v("actual force > position force => apply AW command");
    motor_command = motor_command_range;
  }
  // TODO add PID
}


/**
 * @brief compute the Damper and the Friction effect.
 * Damper force = torque_constant * gain * speed_angular
 * Friction force = friction_constant * gain if speed_angular > 1 tr/s
 * 
 */
uint32_t PedalEngine::computeDamperFriction() {
  uint32_t force_effect;

  // if pedal enfine is not ok, don't compute an effect
  if (state != PE_OK)  {
    return 0;
  }

  const int32_t new_pos = 200;  // TODO read the driver position

  const int32_t current_speed_rps = 
    (position - new_pos) / mm_turn // the delta position in mm / screw pitch => turn/ms
    * 60000;                       // turn/ms => rpm
  speed = current_speed_rps;       // update the class speed

  // compute damper effect
  const uint32_t damper = (speed>0) ? damper_awd : damper_rwd;
  force_effect =  damper_cst * (damper / 200.0) * abs(speed); //TODO add max_speed limit

  // compute friction effect
  if (abs(speed) >= 1) {
    const uint32_t friction = (speed>0) ? friction_awd : friction_rwd;
    force_effect += friction_cst * (friction / 200.0);
  }

  return force_effect;

}

/*********************************** MOVEMENT PROCESSOR **************************************/

/**
 * @brief move the motor to the min endstop, reset the position to the endtop min pos.
 * move to the max position, checkif endstop max is ok and update the max position user if this
 * value is incompatible.
 * 
 * Other option possible : 
 * move the motor to the max position and store the max hardware position
 * move to the mmin, and stop when StallGuard is detected.
 * 
 */
bool PedalEngine::calibration() {
  // integrity check

  // if pedal enfine is not ok, don't compute an effect
  if (state != PE_INIT)  {
    log_e("can't calibrate, the motor is not init");
    return 0;
  }

  bool calibration_success = false;

  /*
  // TODO move motor in RW
  while (!button_min.detect()) {
    delay(10);
    const int32_t new_pos = ;  // TODO read the driver position
    log_v("Homing fw, actual position %d", new_pos);
  }
  log_i("Homing fw, detected %d/%d", driver.XACTUAL(), driver.XTARGET());
  position = endstop_min_mm;

  // TODO move motor in FW
  while (!button_max.detect()) {
    delay(10);
    const int32_t new_pos = ;  // TODO read the driver position
    log_v("Homing fw, actual position %d", new_pos);
  }
  log_i("Homing fw, detected %d/%d", driver.XACTUAL(), driver.XTARGET());
  if (position > endstop_max_mm) {
    state = PE_ONERROR;
    log_e("Position is over the endstop_max");
    return false;
  }
  if (position_max_mm > endstop_max_mm) {
    log_w("force max user position to endstop");
    position_max_mm = endstop_max_mm;
  }*/
  
  state = PE_CALIBRATED;
  return true;
}

/**
 * @brief send the command to the motor. Called it at 1khz.
 * if the calibration is not done, we do nothing
 */
bool PedalEngine::sendMotorCommand() {
  // integrity check
  if (state == PE_OK) return false;

  // send motor_command with motor_command_range gap
  
  return true;
}

/***************************************** ACCESSOR ******************************************/

float PedalEngine::getPosition() {
  return position * mm_turn;
}

float PedalEngine::getPositionMin() {
  return position_min_mm;
}

float PedalEngine::getPositionMax() {
  return position_max_mm;
}

float PedalEngine::getSpeed() {
  return speed;
}


/**
 * @brief change the position_min of the pedal
 * Move the pedal to the new min position 
 * if the min_position is after the current position
 * 
 * @param new_pos new position wanted
 */
void PedalEngine::setPositionMin(float new_pos) {
  // check than new pos is before the position_max
  if (new_pos < position_max_mm) {
    position_min_mm = new_pos;
  } else {
    position_min_mm = position_max_mm;
  }

  // if the new min position is over the current position, plan a move for the pedal
  // TODO move FW
}

/**
 * @brief change the position_max of the pedal
 * Check than we don't go after the endstop
 * Move the pedal to the new max position 
 * if the max_position is before the current position
 * 
 * @param new_pos new position wanted
 */
void PedalEngine::setPositionMax(float new_pos) {
  // check new pos is before the endstop
  if (new_pos > endstop_max_mm) {
    new_pos = endstop_max_mm;
  } 

  // check than new pos is after the position_min
  if (new_pos > position_min_mm) {
    position_max_mm = new_pos;
  } else {
    position_max_mm = position_min_mm;
  }
  
  // if the new min position is over the current position, plan a move for the pedal
  // TODO move RW
}

/**
 * @brief return the actual current
 * 
 * @return uint16_t 
 */
uint16_t PedalEngine::getMotorCommand() {
  return motor_command;
}

/**
 * @brief return the last force recorded normalized for HID
 * 
 * @return uint32_t force in Nm
 */
uint32_t PedalEngine::getHIDValue() {
  // TODO add switch logique position/force.
  return map(force, force_min, force_max, INT16_MIN, INT16_MAX);
  // return map(position, position_min_mm, position_max_mm, INT16_MIN, INT16_MAX);
}