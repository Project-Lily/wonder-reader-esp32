#include "braillecontrol.h"
#include "AccelStepper.h"
#include "driver/gpio.h"
#include "wonderconfig.h"
#include "Preferences.h"
#include "esp_log.h"

#define X_STEPPERS 1

wonder::Stepper x_steppers[X_STEPPERS];

static const char* TAG = "control";

// TODO: Handle when motors timeout homing
void wonder::init_motors() {
  Preferences pref;
  if (!wonder::get_configuration(&pref)) {
    ESP_LOGE(TAG, "Error loading preferences");
    return;
  }

  // Initialize pins
  x_steppers[0] = {
    .stepper = AccelStepper(AccelStepper::FULL2WIRE, 10, 2),
    .home_type = HOMING,
    .limit_pin = GPIO_NUM_10,
  };
  x_steppers[0].stepper.setEnablePin(5);
  x_steppers[0].stepper.setAcceleration(pref.getDouble(wonder::get_config_string(X_ACCEL)));
  x_steppers[0].stepper.setMaxSpeed(pref.getDouble(wonder::get_config_string(X_HOME_SPEED)));

  ESP_LOGI(TAG, "Homing motors");
  ESP_LOGI(TAG, "Homing motor X (%d motor(s))", X_STEPPERS);

  // Home all the x steppers
  while (true) {
    uint8_t homed_count = 0;

    // Delay for 5ms to wait for stepper to move and limit to be pressed
    vTaskDelay(5 / portTICK_PERIOD_MS);

    // Iterate all the X steppers
    for (int i = 0; i < X_STEPPERS; i++) {
      wonder::Stepper st = x_steppers[i];

      switch (st.home_type) {
        case HOMING:
          // If limit is pressed, set stepper to be releasing.
          if (gpio_get_level(st.limit_pin)) {
            ESP_LOGI(TAG, "X Stepper %d hit home", i);
            st.home_type = RELEASING;
          }

          // Move the stepper towards the limit switch
          if (!st.stepper.isRunning()) {
            st.stepper.move(-1);
          }
          st.stepper.run();
          break;

        case RELEASING:
          // If limit is released, set stepper to be homed.
          if (!gpio_get_level(st.limit_pin)) {
            ESP_LOGI(TAG, "X Stepper %d successfully homed", i);
            st.stepper.setCurrentPosition(0);
            st.home_type = HOMED;
            st.stepper.setMaxSpeed(pref.getDouble(wonder::get_config_string(X_SPEED)));
          }
          // Move the stepper towards the limit switch
          if (!st.stepper.isRunning()) {
            st.stepper.move(1);
          }
          st.stepper.run();
          break;

        case HOMED:
        default:
          homed_count++;
          continue;
      }
    }
    if (homed_count == X_STEPPERS) break;
  }

  ESP_LOGI(TAG, "Motors initialized");
}