#ifndef __MOTORCONTROL_H_
#define __MOTORCONTROL_H_

#include "AccelStepper.h"
#include "driver/gpio.h"

// How many braille cells are in each X axis
#define CELL_PER_X_AXIS 9

namespace wonder {
  typedef enum {
    HOMING,     // When the stepper is moving towards the limit
    RELEASING,  // When the stepper is moving towards not pressing the limit
    HOMED,      // When the stepper is done moving
  } HomingType;

  typedef struct {
    AccelStepper stepper;
    HomingType home_type;
    gpio_num_t limit_pin;
  } Stepper;

  void _home_x(double x_home_speed, double x_norm_speed);

  // Initializes the motors
  void init_motors();
  void home_x();
}

#endif