#ifndef __MOTORCONTROL_H_
#define __MOTORCONTROL_H_

#include "AccelStepperSR.h"
#include "driver/gpio.h"

// How many braille cells are in each X axis
#define CELL_PER_X_AXIS 9

namespace wonder {
  typedef enum {
    HOMING,     // When the stepper is moving towards the limit
    RELEASING,  // When the stepper is moving towards not pressing the limit
    HOMED,      // When the stepper is done moving
  } HomingType;

  
  template<uint8_t S>
  struct Stepper {
    AccelStepperSR<S> stepper;
    HomingType home_type;
    gpio_num_t limit_pin;
  };

  void _home_x(double x_home_speed, double x_norm_speed);
  void _home_y(double y_speed, long max_home_duration);

  // Initializes the motors
  void init_motors();
  void home_x();
  void home_y();
}

#endif