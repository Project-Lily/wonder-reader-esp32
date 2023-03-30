#include "braillecontrol.h"
#include "AccelStepperSR.h"
#include "driver/gpio.h"
#include "wonderconfig.h"
#include "Preferences.h"
#include "esp_log.h"
#include "ShiftRegister74HC595.h"
#include "brailletranslate/brailletranslation.h"
#include <Keypad.h>

#define X_STEPPERS 1
#define Y_STEPPERS 1
#define DEBOUNCE_DELAY 1000 // ms
#define Y_HOME_DURATION 1200 // ms

#define BRAILLE_PINS 3 // Braille pin per cell
#define Y_ENABLE_PIN GPIO_NUM_19

#define Y_RESTING_POS 1450
#define Y_DOWN_POS 2200
#define Y_UP_POS 0

#define X_PIN_NEIGHBOUR_DIST 1530
#define X_PIN_ADJA_DIST 2550

ShiftRegister74HC595<1> sr_x(16, 2, 4);
wonder::Stepper<1> x_steppers[X_STEPPERS] = {{
  // NOTE: pins here are for the shift register
  .stepper = AccelStepperSR<1>(&sr_x, AccelStepperSR<1>::FULL4WIRE, 2, 4, 0, 3),
  .home_type = wonder::HOMING,
  .limit_pin = GPIO_NUM_21,
}};

ShiftRegister74HC595<2> sr_y(18, 5, 17);
AccelStepperSR<2> y_steppers[Y_STEPPERS][BRAILLE_PINS] = {
  {
    // AccelStepperSR<2>(&sr_y, AccelStepperSR<2>::DRIVER, 0, 1),
    // AccelStepperSR<2>(&sr_y, AccelStepperSR<2>::DRIVER, 2, 3),
    AccelStepperSR<2>(&sr_y, AccelStepperSR<2>::DRIVER, 4, 5),
    AccelStepperSR<2>(&sr_y, AccelStepperSR<2>::DRIVER, 6, 7),
    // AccelStepperSR<2>(&sr_y, AccelStepperSR<2>::DRIVER, 8, 9),
    AccelStepperSR<2>(&sr_y, AccelStepperSR<2>::DRIVER, 10, 11),
  },
};

static const char* TAG = "control";
const char* text = "";
int offset = 5;

void run_y_until_done() {
  for (int i = 0; i < Y_STEPPERS; i++) {
    for (int j = 0; j < BRAILLE_PINS; j++) {
      y_steppers[i][j].enableOutputs();
    }
  }

  while (1) {
    bool is_end = true;
    for (int i = 0; i < Y_STEPPERS; i++) {
      for (int j = 0; j < BRAILLE_PINS; j++) {
        if (y_steppers[i][j].distanceToGo() != 0) {
          y_steppers[i][j].run();
          is_end = false;
        }
      }
    }
    if (is_end) break;
  }

  for (int i = 0; i < Y_STEPPERS; i++) {
    for (int j = 0; j < BRAILLE_PINS; j++) {
      y_steppers[i][j].disableOutputs();
    }
  }
}

void run_x_until_done() {
  while (1) {
    bool is_end = true;
    for (int i = 0; i < X_STEPPERS; i++) {
      if (x_steppers[i].stepper.distanceToGo() != 0) {
        x_steppers[i].stepper.run();
        is_end = false;
      }
    }
    if (is_end) break;
  }
}

void wonder::_home_y(double y_home_speed, double y_speed, long max_home_duration) {
  long home_until = (esp_timer_get_time() / 1000) + max_home_duration;

  ESP_LOGI(TAG, "[%dms] Homing Y motors", esp_log_timestamp());
  // Sets speed
  for (int i = 0; i < Y_STEPPERS; i++) {
    for (int j = 0; j < BRAILLE_PINS; j++) {
      y_steppers[i][j].setMaxSpeed(y_home_speed);
      y_steppers[i][j].setSpeed(-y_home_speed);
      y_steppers[i][j].enableOutputs();
    }
  }

  // Home all the y steppers. Because they do not have limit switches,
  // We just force all the steppers to the edge.
  while ((esp_timer_get_time() / 1000) <= home_until) {
    for (int i = 0; i < Y_STEPPERS; i++) {
      for (int j = 0; j < BRAILLE_PINS; j++) {
        y_steppers[i][j].runSpeed();
      }
    }
  }

  // Get the stepper to a stable location
  for (int i = 0; i < Y_STEPPERS; i++) {
    for (int j = 0; j < BRAILLE_PINS; j++) {
      y_steppers[i][j].setCurrentPosition(0);
      y_steppers[i][j].setMaxSpeed(y_speed);
      y_steppers[i][j].setSpeed(y_speed);
      y_steppers[i][j].moveTo(Y_DOWN_POS);
    }
  }

  run_y_until_done();

  for (int i = 0; i < Y_STEPPERS; i++) {
    for (int j = 0; j < BRAILLE_PINS; j++) {
      y_steppers[i][j].moveTo(Y_RESTING_POS);
    }
  }

  run_y_until_done();

  ESP_LOGI(TAG, "[%dms] Y motors homed", esp_log_timestamp());
}

void wonder::home_y() {
  Preferences pref;
  if (!wonder::get_configuration(&pref)) {
    ESP_LOGE(TAG, "Error loading preferences");
    return;
  }

  double y_home_speed = pref.getDouble(wonder::get_config_string(Y_HOME_SPEED));
  double y_speed = pref.getDouble(wonder::get_config_string(Y_SPEED));
  _home_y(y_home_speed, y_speed, Y_HOME_DURATION);
}

// TODO: Handle when motors timeout homing
void wonder::_home_x(double x_home_speed, double x_norm_speed) {
  x_steppers[0].stepper.setMaxSpeed(x_home_speed);

  // Set speed to be minus because we are homing
  x_steppers[0].stepper.setSpeed(x_home_speed);

  ESP_LOGI(TAG, "[%dms] Homing motor X (%d motor(s))", esp_log_timestamp(), X_STEPPERS);

  // To keep track when motors should run
  uint64_t x_run_milli[X_STEPPERS];
  for (int i = 0; i < X_STEPPERS; i++) {
    x_run_milli[i] = 0;
  }

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
      wonder::Stepper<1>* st = &x_steppers[i];

      switch (st->home_type) {
        case HOMING:
          // If limit is pressed, set stepper to be releasing.
          if (gpio_get_level(st->limit_pin)) {
            ESP_LOGI(TAG, "[%dms] X Stepper %d hit home", esp_log_timestamp(), i);
            st->home_type = RELEASING;
            st->stepper.setSpeed(-x_home_speed);
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
            st->stepper.disableOutputs();
            st->home_type = HOMED;
            st->stepper.setSpeed(-x_norm_speed);
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
  ESP_LOGI(TAG, "[%dms]: X motor homed", esp_log_timestamp());
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

void wonder::init_motors() {
  Preferences pref;
  if (!wonder::get_configuration(&pref)) {
    ESP_LOGE(TAG, "Error loading preferences");
    return;
  }

  // Settings
  // Setting x stepper
  x_steppers[0].stepper.setAcceleration(pref.getDouble(wonder::get_config_string(X_ACCEL)));

  // Setting y stepper
  for (int i = 0; i < X_STEPPERS; i++) {
    for (int j = 0; j < BRAILLE_PINS; j++) {
      y_steppers[i][j].setAcceleration(pref.getDouble(wonder::get_config_string(Y_ACCEL)));
      y_steppers[i][j].setPinsInverted(false, false, true);
      y_steppers[i][j].setEnablePin(Y_ENABLE_PIN, false);
      y_steppers[i][j].disableOutputs();
    }
  }

  ESP_LOGI(TAG, "[%dms] Init motors", esp_log_timestamp());

  _home_y(
    pref.getDouble(wonder::get_config_string(Y_HOME_SPEED)),
    pref.getDouble(wonder::get_config_string(Y_SPEED)),
    Y_HOME_DURATION
  );

  _home_x(
    pref.getDouble(wonder::get_config_string(X_HOME_SPEED)),
    pref.getDouble(wonder::get_config_string(X_SPEED))
  );

  // Set initial text
  uint8_t buffer[9] = {0};
  for (int i = 0; i < 9; i++) {
    ESP_LOGI(TAG, "bufferino %d: %d", i, buffer[i]);
  }
  wonder::brailleTranslation("halo", 0, 9, buffer);
  uint8_t until = 0;
  for (int i = 0; i < 9; i++) {
    ESP_LOGI(TAG, "no %d: %d", i, buffer[i]);
    if (buffer[i] != 0) {
      until = i + 1;
    }
  }

  for (int i = 0; i < until; i++) {
    uint8_t first = buffer[i] & 7;
    y_steppers[0][1].moveTo((first & 1) ? Y_UP_POS : Y_DOWN_POS);
    y_steppers[0][2].moveTo(((first & 2) >> 1) ? Y_UP_POS : Y_DOWN_POS);
    y_steppers[0][0].moveTo(((first & 4) >> 2) ? Y_UP_POS : Y_DOWN_POS);

    run_y_until_done();

    for (int j = 0; j < BRAILLE_PINS; j++) {
      y_steppers[0][j].moveTo(Y_RESTING_POS);
    }

    run_y_until_done();

    x_steppers[0].stepper.move(-X_PIN_NEIGHBOUR_DIST);
    run_x_until_done();

    uint8_t second = buffer[i] & 56;
    y_steppers[0][1].moveTo(((second & 8) >> 3) ? Y_UP_POS : Y_DOWN_POS);
    y_steppers[0][2].moveTo(((second & 16) >> 4) ? Y_UP_POS : Y_DOWN_POS);
    y_steppers[0][0].moveTo(((second & 32) >> 5) ? Y_UP_POS : Y_DOWN_POS);

    run_y_until_done();

    for (int j = 0; j < BRAILLE_PINS; j++) {
      y_steppers[0][j].moveTo(Y_RESTING_POS);
    }

    run_y_until_done();

    x_steppers[0].stepper.move(-X_PIN_ADJA_DIST);
    run_x_until_done();

  }
  
  ESP_LOGI(TAG, "[%dms] Motors initialized", esp_log_timestamp());
}