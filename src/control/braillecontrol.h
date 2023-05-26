#ifndef __MOTORCONTROL_H_
#define __MOTORCONTROL_H_

#include "AccelStepperSR.h"
#include "driver/gpio.h"
#include <string>

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
  void _home_y(double y_home_speed, double y_speed, long max_home_duration);

  // Initializes the motors
  void init_motors();
  void home_x();
  void home_y();

  void display_text(std::string text);
  void braille_task(void *pvParameters);
  void send_letter(uint8_t braille_letter);
  void up_page();
  void down_page();
  void backspace();

  std::string get_current_text();
}

#endif