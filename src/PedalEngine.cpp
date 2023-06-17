#include "PedalEngine.h"

PedalEngine::PedalEngine() {

};

bool PedalEngine::begin() {
  bool result;
  driver.begin();                           // Initiate pins (inc. CS) and registeries

  if (!driver.isEnabled()) {
    log_e("TMC is not enabled !");
    return false;
  }
  driver.rms_current(current_max, 0.5);     // Set stepper current to 600mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  driver.en_pwm_mode(1);                    // Enable extremely quiet stepping
  driver.pwm_autoscale(1);                  // Enable autocalibration, done during home
  driver.microsteps(mstep);                 // set the mstep to 8
  
  driver.RAMPMODE(3);                       // hold the motor in the current position

  log_v("TMC Current %d/%d",driver.rms_current(), current_max);
  log_v("TMC PWM Mode %d",driver.en_pwm_mode());
  log_v("TMC PWM Autoscale %d",driver.pwm_autoscale());
  log_v("TMC Microstep %d",driver.microsteps());
  log_v("TMC Enable %d",driver.drv_enn());

  // check if the driver config is ok after read its
  result = (driver.rms_current() > current_max * 0.9) &&
           (driver.en_pwm_mode() == 1)  &&
           (driver.pwm_autoscale() == 1)  &&
           (driver.microsteps() == mstep);

  log_d("tmc init and enable %d", result, driver.isEnabled());

  const bool isInitOK = result && driver.isEnabled();
  state = isInitOK ? PE_INIT : PE_ONERROR;
  return isInitOK;
}

bool PedalEngine::setupRamp() {
  bool result = false;

  driver.VSTART(10);                        // 10 step/s
  driver.a1(accel_max_spt2 * mstep / 8);    // 800 step/s²
  driver.v1(speed_max_spt * mstep);         // 20000 step/s
  driver.AMAX(accel_max_spt2 * mstep);      // 6400 step/s²
  driver.DMAX(accel_max_spt2 * mstep);      // 6400 step/s²
  driver.d1(accel_max_spt2 * mstep / 8);    // 800 step/s²
  driver.VSTOP(10);                         // 10 step/s

  log_v("TMC RampUP VSTART/a1/v1 : %d/%d/%d", driver.VSTART(), driver.a1(), driver.v1());
  log_v("TMC PWM Mode %d",driver.en_pwm_mode());
  log_v("TMC PWM Autoscale %d",driver.pwm_autoscale());
  log_v("TMC Microstep %d",driver.microsteps());
  log_v("TMC Enable %d",driver.drv_enn());


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
  position_target_s = map(new_force, force_min, force_max, position_min_s, position_max_s);
}


/**
 * @brief compute the Damper and the Friction effect.
 * Damper force = torque_constant * gain * speed_angular
 * Friction force = friction_constant * gain if speed_angular > 1 tr/s
 * 
 */
uint32_t PedalEngine::computeDamperFriction() {
  uint32_t force_effect;

  // if stepper is not ready, don't compute an effect
  if (!driver.isEnabled())  {
    return 0;
  }

  const int32_t current_speed_rps = driver.VACTUAL(); // read the speed in ustep/s
  speed = driver.VACTUAL(); // read the speed in ustep/s

  // compute damper effect
  const uint32_t damper = (speed>0) ? damper_awd : damper_rwd;
  force_effect =  damper_cst * (damper / 200.0) * abs(speed);

  // compute friction effect
  if (abs(speed) >= 1) {
    const uint32_t friction = (speed>0) ? friction_awd : friction_rwd;
    force_effect += friction_cst * (friction / 200.0);
  }

  return force_effect;

}

/*********************************** MOVEMENT PROCESSOR **************************************/

/**
 * @brief move the motor to the max position and store the max hardware position
 * move to the mmin, and stop when StallGuard is detected.
 * 
 * The homing of a linear drive requires moving the motor into the direction of a hard stop. 
 * As StallGuard needs a certain velocity to work (as set by TCOOLTHRS), make sure that the start point is far enough away 
 * from the hard stop to provide the distance required for the acceleration phase. After setting up SGT and 
 * the ramp generator registers, start a motion into the direction of the hard stop and activate the stop on stall function (set sg_stop in SW_MODE). 
 * Once a stall is detected, the ramp generator stops motion and sets VACTUAL zero, stopping the motor. 
 * The stop condition also is indicated by the flag StallGuard in DRV_STATUS. 
 * After setting up new motion parameters to prevent the motor from restarting right away, StallGuard can be disabled, 
 * or the motor can be re-enabled by reading and writing back RAMP_STAT. The write and clear function of the event_stop_sg flag 
 * in RAMP_STAT restarts the motor after expiration of TZEROWAIT in case the motion parameters have not been modified. 
 * Best results are yielded at 30% to 70% of nominal motor current and typically 1 to 5 RPS (motors smaller than NEMA17 may require higher velocities).
 * 
 */
bool PedalEngine::calibration() {
  // integrity check
  if (!driver.isEnabled()) {
    log_e("can't calibrate, the stepper is not init");
    return false;
  }

  driver.TCOOLTHRS(55); // sample sur blog tmc http://blog.trinamic.com/2019/02/22/explore-and-tune-stallguard2-with-the-tmc5160-eval-arduino-mega-2560-in-step-and-direction-mode/
  driver.sgt(0);        // Stallguard level to middle
  driver.sg_stop(true); // Stop on stallguard and reset the position
  driver.RAMPMODE(1);   // Change mode to go in 
  driver.VMAX(1*step_turn); // start the move

  while (!driver.stallguard()) {
    delay(100);
    log_v("Homing fw, actual position %d", driver.XACTUAL());
  }
  driver.RAMPMODE(3);
  log_i("Homing fw, detected %d/%d", driver.XACTUAL(), driver.XTARGET());

  delay(10000000);


  // TODO remove dev
  log_w("force calibration for dev");
  state = PE_CALIBRATED;
  return true;

  bool calibration_success = true;
  // TODO add  calibration issue detection
  
  // Run to home
  //TODO manage calibration driver.en_softstop(true);// stepper->runBackward();
  driver.XTARGET(INT32_MIN);
  while (digitalRead(HOME_PIN_ENDSTOP) || driver.stallguard()) {
    delay(1);
  }
  driver.XTARGET(driver.XACTUAL());
  endstop_min_s = driver.XACTUAL();

  // Run to endstop
  driver.XTARGET(INT32_MAX);
  while (digitalRead(END_PIN_ENDSTOP) || driver.stallguard()) {
    delay(1);
  }
  driver.XTARGET(driver.XACTUAL());
  endstop_max_s = driver.XACTUAL();

  driver.XTARGET(position_min_s);

  // TODO check if init ok
  state = PE_CALIBRATED;
  
  return calibration_success;
}

/**
 * @brief send the command to the motor. Called it at 1khz.
 * if the calibration is not done, we do nothing
 */
bool PedalEngine::sendMotorCommand() {
  // integrity check
  if (state == PE_OK) return false;

  // read the metrics TODO move it in a readmetrics process
  position_s = driver.XACTUAL();
  
  driver.XTARGET(position_target_s);
  //TODO add a position sensor and make constant update
  
  return true;
}

/***************************************** ACCESSOR ******************************************/

float PedalEngine::getPosition() {
  return position_s * mm_turn / float(step_turn);
}

float PedalEngine::getPosition_target() {
  return position_target_s * mm_turn / float(step_turn);
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
  position_min_s = position_min_mm * step_turn / mm_turn;

  // if the new min position is over the current position, plan a move for the pedal
  if (position_min_s > position_s) {
    position_target_s = position_min_s;
    loop();
  }
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
  position_max_s = position_max_mm * step_turn / mm_turn;
  
  // if the new min position is over the current position, plan a move for the pedal
  if (position_max_s < position_s) {
    position_target_s = position_max_s;
    loop();
  }
}

/**
 * @brief return the actual current
 * 
 * @return uint16_t 
 */
uint16_t PedalEngine::getCurrent() {
  return driver.cs2rms(driver.cs_actual());
}

/**
 * @brief return the last force recorded normalized for HID
 * 
 * @return uint32_t force in Nm
 */
uint32_t PedalEngine::getHIDValue() {
  // TODO add force/position answer
  return map(force, force_min, force_max, INT16_MIN, INT16_MAX);
}