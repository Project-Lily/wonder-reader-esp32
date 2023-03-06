#include "braillecontrol.h"
#include "AccelStepper.h"
#include "driver/gpio.h"
#include "wonderconfig.h"
#include "Preferences.h"
#include "esp_log.h"
#include "ShiftRegister74HC595.h"

#define X_STEPPERS 1
#define DEBOUNCE_DELAY 1000 // ms

wonder::Stepper x_steppers[X_STEPPERS];
ShiftRegister74HC595<1> sr_x(15, 2, 4);

static const char* TAG = "control";
const char* text = "";
int offset = 5;

void wonder::_home_x(double x_home_speed, double x_norm_speed) {
  x_steppers[0].stepper.setMaxSpeed(x_home_speed);

  // Set speed to be minus because we are homing
  x_steppers[0].stepper.setSpeed(-x_home_speed);

  ESP_LOGI(TAG, "[%dms] Homing motors", esp_log_timestamp());
  ESP_LOGI(TAG, "[%dms] Homing motor X (%d motor(s))", esp_log_timestamp(), X_STEPPERS);

  // To keep track when motors should run
  uint64_t x_run_milli[X_STEPPERS];

  // Home all the x steppers
  while (true) {
    uint8_t homed_count = 0;

    // Delay to wait for stepper to move and limit to be pressed
    vTaskDelay(2 / portTICK_PERIOD_MS);

    // Iterate all the X steppers
    for (int i = 0; i < X_STEPPERS; i++) {
      uint64_t milli = esp_timer_get_time() / 1000;
      if (milli < x_run_milli[i]) continue;

      // NOTE: Has to be a reference. Normal variable won't work
      wonder::Stepper* st = &x_steppers[i];

      switch (st->home_type) {
        case HOMING:
          // If limit is pressed, set stepper to be releasing.
          if (gpio_get_level(st->limit_pin)) {
            ESP_LOGI(TAG, "[%dms] X Stepper %d hit home", esp_log_timestamp(), i);
            st->home_type = RELEASING;
            st->stepper.setSpeed(x_home_speed);
            x_run_milli[i] = milli + DEBOUNCE_DELAY;
          }

          // Move the stepper towards the limit switch
          st->stepper.runSpeed();
          break;

        case RELEASING:
          // If limit is released, set stepper to be homed.
          if (!gpio_get_level(st->limit_pin)) {
            ESP_LOGI(TAG, "[%dms]: X Stepper %d successfully homed", esp_log_timestamp(), i);
            st->stepper.setCurrentPosition(0);
            st->home_type = HOMED;
            st->stepper.setMaxSpeed(x_norm_speed);
          }
          // Move the stepper towards the limit switch
          st->stepper.runSpeed();
          break;

        case HOMED:
        default:
          homed_count++;
          continue;
      }
    }
    if (homed_count == X_STEPPERS) break;
  }
  ESP_LOGI(TAG, "Carriage motor homed");
}

void wonder::home_x() {
  Preferences pref;
  if (!wonder::get_configuration(&pref)) {
    ESP_LOGE(TAG, "Error loading preferences");
    return;
  }

  double x_home_speed = pref.getDouble(wonder::get_config_string(X_HOME_SPEED));
  double x_norm_speed = pref.getDouble(wonder::get_config_string(X_SPEED));
  _home_x(x_home_speed, x_norm_speed);
}

// TODO: Handle when motors timeout homing
void wonder::init_motors() {
  Preferences pref;
  if (!wonder::get_configuration(&pref)) {
    ESP_LOGE(TAG, "Error loading preferences");
    return;
  }

  // Initialize pins
  x_steppers[0] = {
    // NOTE: pins here are for the shift register
    .stepper = AccelStepper(&sr_x, AccelStepper::FULL4WIRE, 0, 1, 2, 3),
    .home_type = HOMING,
    .limit_pin = GPIO_NUM_5,
  };

  x_steppers[0].stepper.setAcceleration(pref.getDouble(wonder::get_config_string(X_ACCEL)));
  _home_x(
    pref.getDouble(wonder::get_config_string(X_HOME_SPEED)),
    pref.getDouble(wonder::get_config_string(X_SPEED))
  );
  
  ESP_LOGI(TAG, "Motors initialized");
}