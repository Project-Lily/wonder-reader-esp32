#include "braillecontrol.h"
#include "AccelStepperSR.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "wonderconfig.h"
#include "Preferences.h"
#include "esp_log.h"
#include "ShiftRegister74HC595.h"
#include "brailletranslate/brailletranslation.h"
#include <Keypad.h>
#include <string>
#include <vector>

#define DEBOUNCE_DELAY 1000 // ms
#define Y_HOME_DURATION 1200 // ms

// Stepper motor steps / mm
// 472.4 steps/mm
#define Y_ENABLE_PIN 6
#define Y_RESTING_POS -1300
#define Y_DOWN_POS 0
#define Y_UP_POS -1600

#define X_PIN_NEIGHBOUR_DIST -130
#define X_PIN_ADJA_DIST -245
#define X_PIN_OFFSET 3500
#define X_ENABLE_PIN GPIO_NUM_22

// How many braille cells are in each X axis
#define TOTAL_CELLS 9

#define BRAILLE_CELL_ROWS 3

#define KEY_BIT BIT0

static uint32_t display_offset = 0;
static std::vector<uint8_t> full_text;
static uint8_t target_display[TOTAL_CELLS] = {0};
static uint8_t current_display[TOTAL_CELLS] = {0};
static EventGroupHandle_t display_event;
static uint32_t cursor_pos = 0;
static const char* TAG = "movement";

static wonder::Stepper<1> x_stepper = {
  // NOTE: pins here are for the shift register
  .stepper = AccelStepperSR<1>(nullptr, AccelStepperSR<1>::DRIVER, 19, 18),
  .home_type = wonder::HOMING,
  .limit_pin = GPIO_NUM_21,
};

static ShiftRegister74HC595<1> sr_y(16, 5, 17);
static AccelStepperSR<1> y_steppers[BRAILLE_CELL_ROWS] = {
  AccelStepperSR<1>(&sr_y, AccelStepperSR<1>::DRIVER, 1, 0),
  AccelStepperSR<1>(&sr_y, AccelStepperSR<1>::DRIVER, 3, 2),
  AccelStepperSR<1>(&sr_y, AccelStepperSR<1>::DRIVER, 5, 4)
};

std::string wonder::get_current_text() {
  return wonder::braille_to_text(full_text.data(), full_text.size());
}

static void run_y_until_done() {
  for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
    y_steppers[i].enableOutputs();
  }

  bool is_end = false;
  while (!is_end) {
    is_end = true;
    for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
      if (y_steppers[i].distanceToGo() != 0) {
        y_steppers[i].run();
        is_end = false;
      }
    }
  }

  for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
    y_steppers[i].disableOutputs();
  }
}

static void run_x_until_done() {
  x_stepper.stepper.enableOutputs();
  while (true) {
    if (x_stepper.stepper.distanceToGo() == 0) break;
    x_stepper.stepper.run();
  }
}

void wonder::_home_y(double y_home_speed, double y_speed, long max_home_duration) {
  long home_until = (esp_timer_get_time() / 1000) + max_home_duration;

  ESP_LOGI(TAG, "[%dms] Homing Y motors", esp_log_timestamp());

  // Sets speed
  for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
    y_steppers[i].setMaxSpeed(y_home_speed);
    y_steppers[i].setSpeed(-y_home_speed);
    y_steppers[i].enableOutputs();
  }

  // Get the stepper to a stable location
  for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
    y_steppers[i].setCurrentPosition(0);
    y_steppers[i].setMaxSpeed(y_speed);
    y_steppers[i].setSpeed(y_speed);
    y_steppers[i].moveTo(-Y_UP_POS);
  }

  run_y_until_done();

  for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
    y_steppers[i].setCurrentPosition(0);
    y_steppers[i].moveTo(Y_RESTING_POS);
  }

  run_y_until_done();

  ESP_LOGI(TAG, "[%dms] Y motors homed", esp_log_timestamp());
}

static void change_letter(uint8_t letter) {
  // Move the first set of the braille pins
    uint8_t first = letter & 7;
    y_steppers[1].moveTo((first & 1) ? Y_UP_POS : Y_DOWN_POS);
    y_steppers[2].moveTo(((first & 2) >> 1) ? Y_UP_POS : Y_DOWN_POS);
    y_steppers[0].moveTo(((first & 4) >> 2) ? Y_UP_POS : Y_DOWN_POS);

    run_y_until_done();

    for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
      y_steppers[i].moveTo(Y_RESTING_POS);
    }

    run_y_until_done();

    // Move x to next cell
    x_stepper.stepper.move(-X_PIN_NEIGHBOUR_DIST);
    run_x_until_done();

    // Move the second set of the braille pins
    uint8_t second = letter & 56;
    y_steppers[1].moveTo(((second & 8) >> 3) ? Y_UP_POS : Y_DOWN_POS);
    y_steppers[2].moveTo(((second & 16) >> 4) ? Y_UP_POS : Y_DOWN_POS);
    y_steppers[0].moveTo(((second & 32) >> 5) ? Y_UP_POS : Y_DOWN_POS);

    run_y_until_done();

    for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
      y_steppers[i].moveTo(Y_RESTING_POS);
    }

    run_y_until_done();
}

static void move_by_letters(int n) {
  if (cursor_pos + n <= TOTAL_CELLS || cursor_pos + n >= 0) {
    x_stepper.stepper.move(-X_PIN_ADJA_DIST * n);
    run_x_until_done();
  }
}

static void go_to_position(uint32_t position) {
  x_stepper.stepper.move((cursor_pos - position) * X_PIN_ADJA_DIST);
  run_x_until_done();
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
  x_stepper.stepper.setMaxSpeed(x_home_speed);

  // Set speed to be minus because we are homing
  x_stepper.stepper.setSpeed(x_home_speed);

  ESP_LOGI(TAG, "[%dms] Homing motor X", esp_log_timestamp());

  // To keep track when motors should run
  uint64_t x_run_milli = 0;

  // Home all the x steppers
  bool homed = false;
  while (!homed) {
    uint64_t milli = esp_timer_get_time() / 1000;

    // NOTE: Has to be a reference. Normal variable won't work
    wonder::Stepper<1>* st = &x_stepper;

    switch (st->home_type) {
      case HOMING:
        // If limit is pressed, set stepper to be releasing.
        if (gpio_get_level(st->limit_pin)) {
          ESP_LOGI(TAG, "[%dms] X Stepper hit home", esp_log_timestamp());
          st->home_type = RELEASING;
          st->stepper.setSpeed(-x_home_speed);
          x_run_milli = milli + DEBOUNCE_DELAY;
        }

        // Move the stepper towards the limit switch
        st->stepper.runSpeed();
        break;

      case RELEASING:
        // If limit is released, set stepper to be homed.
        if (!gpio_get_level(st->limit_pin)) {
          ESP_LOGI(TAG, "[%dms]: X Stepper successfully homed", esp_log_timestamp());
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
        homed = true;
        continue;
    }
  }
  
  ESP_LOGI(TAG, "[%dms] Home Success", esp_log_timestamp());
  x_stepper.stepper.setCurrentPosition(0);
  x_stepper.stepper.move(-X_PIN_OFFSET);
  run_x_until_done();
  x_stepper.stepper.setCurrentPosition(0);

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
  display_event = xEventGroupCreate();
  Preferences pref;
  if (!wonder::get_configuration(&pref)) {
    ESP_LOGE(TAG, "Error loading preferences");
    return;
  }

  // Setting x stepper
  x_stepper.stepper.setAcceleration(pref.getDouble(wonder::get_config_string(X_ACCEL)));
  x_stepper.stepper.setPinsInverted(false, false, true);
  x_stepper.stepper.setEnablePin(X_ENABLE_PIN, false);

  // Setting y stepper
  for (int i = 0; i < BRAILLE_CELL_ROWS; i++) {
    y_steppers[i].setAcceleration(pref.getDouble(wonder::get_config_string(Y_ACCEL)));
    y_steppers[i].setPinsInverted(false, false, true);
    y_steppers[i].setEnablePin(Y_ENABLE_PIN, true);
    y_steppers[i].disableOutputs();
  }

  // Special case
  y_steppers[1].setPinsInverted(true, false, true);

  ESP_LOGI(TAG, "[%dms] Init motors", esp_log_timestamp());

  #ifndef TEST_NO_MOVE_MODE
  _home_y(
    pref.getDouble(wonder::get_config_string(Y_HOME_SPEED)),
    pref.getDouble(wonder::get_config_string(Y_SPEED)),
    Y_HOME_DURATION
  );

  _home_x(
    pref.getDouble(wonder::get_config_string(X_HOME_SPEED)),
    pref.getDouble(wonder::get_config_string(X_SPEED))
  );
  #endif
  
  ESP_LOGI(TAG, "[%dms] Motors initialized", esp_log_timestamp());
}

void wonder::braille_task(void *pvParameters) {
  while (1) {
    // Wait for signal
    xEventGroupWaitBits(display_event, KEY_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

    while (true) {
      // from the beginning, iterate if target and current display is the same
      bool complete = true;
      int to_change;
      for (to_change = 0; to_change < TOTAL_CELLS; to_change++) {
        if (current_display[to_change] != target_display[to_change]) {
          complete = false;
          break;
        }
      }
      if (complete) break;

      ESP_LOGI(TAG, "Changing pos %d to %d", to_change, target_display[to_change]);

      go_to_position(to_change);
      change_letter(target_display[to_change]);
      current_display[to_change] = target_display[to_change];
      cursor_pos = to_change;

      // Give back control to scheduler
      vTaskDelay(pdMS_TO_TICKS(100));
    }

    // If cursor pos is not at the end, set to next unset letter
    if (cursor_pos < TOTAL_CELLS-1) {
      go_to_position(cursor_pos++);
    }

    ESP_LOGI(TAG, "Algo done! Text now: ");
    for (int i = 0; i < TOTAL_CELLS; i++) {
      ESP_LOGI(TAG, "%d", current_display[i]);
    }
  }
}

void update_target_display(int new_offset) {
  // New display offset
  display_offset = new_offset;
  int max_offset = (full_text.size() - 1) / TOTAL_CELLS;

  // Se the beginning of the pointer and end.
  // This variable tells if the current offset displays the end of the text.
  bool is_at_end = display_offset < max_offset;
  int len_to_read = TOTAL_CELLS;
  if (is_at_end) {
    len_to_read = full_text.size() - ((max_offset+1) * TOTAL_CELLS);
  }

  auto begin = full_text.begin() + (display_offset * TOTAL_CELLS);
  auto end = is_at_end
  ? full_text.end()
  : full_text.begin() + (display_offset * TOTAL_CELLS) + len_to_read;

  std::copy(begin, end, target_display);

  if (is_at_end) {
    std::fill_n(target_display + len_to_read, TOTAL_CELLS-len_to_read, 0);
  }
  xEventGroupSetBits(display_event, KEY_BIT);
}

static void update_target_display_end() {
  update_target_display((full_text.size() - 1) / TOTAL_CELLS);
}

void wonder::up_page() {
  int max_offset = (full_text.size() - 1) / TOTAL_CELLS;
  if (display_offset > 0) {
    update_target_display(--display_offset);
  }
}

void wonder::down_page() {
  int max_offset = (full_text.size() - 1) / TOTAL_CELLS;
  if (display_offset < max_offset) {
    update_target_display(++display_offset);
  }
}

void wonder::send_letter(uint8_t letter) {
  full_text.push_back(letter);
  update_target_display_end();
}

void wonder::backspace() {
  if (full_text.size() == 0) return;
  full_text.pop_back();
  update_target_display_end();
}

void wonder::display_text(std::string new_text) {
  display_offset = 0;
  uint8_t buffer[new_text.length()];
  text_to_braille(new_text, 0, new_text.length(), buffer);
  full_text.clear();
  full_text.insert(full_text.end(), buffer, buffer + new_text.length());
  update_target_display_end();
}