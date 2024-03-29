#include "keyboard.h"
#include "Keypad.h"
#include "esp_log.h"
#include "control/braillecontrol.h"
#include "sound.h"
#include "network.h"

#define KEYBOARD_ROWS 4
#define KEYBOARD_COLS 4

#define WAIT_DURATION 66

enum class BrailleKeyState
{
  IDLE,
  PRESSED,
  KEYSENT
};

const char *TAG = "keyboard";

char keys[KEYBOARD_ROWS][KEYBOARD_COLS] = {
    {'4', 'a', 'p', '?'}, // 35
    {'5', 'w', '3', ' '}, // 34
    {'6', 's', '2', 'b'}, // 39
    {'e', 'd', '1', '/'}  // 36
    //  2    27   33   32
};

byte row_pins[KEYBOARD_ROWS] = {35, 34, 39, 36};
byte col_pins[KEYBOARD_COLS] = {2, 27, 33, 32};
Keypad __keyboard = Keypad(makeKeymap(keys), row_pins, col_pins, KEYBOARD_ROWS, KEYBOARD_COLS);

// When any braille key is first clicked (For combinations)
uint64_t send_key_time = 0;
BrailleKeyState braille_key_state = BrailleKeyState::IDLE;

void wonder::init_board() {
  __keyboard.setDebounceTime(30);
}

static void press_key(char character)
{
  switch (character)
  {
  case 'b':
    wonder::backspace();
    break;
  case ' ':
    wonder::send_letter(0);
    break;
  case 'p':
    ESP_LOGI(TAG, "Playing text: %s", wonder::get_current_text().c_str());
    wonder::play_text(wonder::get_current_text());
    ESP_LOGI(TAG, "Playing done");
    break;
  case 'w':
    wonder::up_page();
    break;
  case 's':
    wonder::down_page();
    break;
  case 'e':
    wonder::send_answer_bt();
    break;
  case '/':
    wonder::switch_answer_mode();
    break;
  default:
    break;
  }
}

void wonder::process_events()
{
  uint64_t cur_milli = esp_timer_get_time() / 1000;

  // If timer has passed since braille key(s) is clicked
  if (braille_key_state == BrailleKeyState::PRESSED && cur_milli > send_key_time)
  {
    braille_key_state = BrailleKeyState::KEYSENT;
    // Get keys which is pressed
    uint8_t brl_num = 0;
    for (int i = 0; i < LIST_MAX; i++)
    {
      Key key = __keyboard.key[i];
      if (key.kchar > 48 && key.kchar < 55 && (key.kstate == PRESSED || key.kstate == HOLD))
      {
        // Char conversion to number
        char brl_key = key.kchar ^ 48;
        // ESP_LOGI(TAG, "brl_pressed: %d", brl_key);
        brl_num += 1 << (brl_key - 1);
      }
    }

    ESP_LOGI(TAG, "sent: %d", brl_num);
    wonder::send_letter(brl_num);
    ESP_LOGI(TAG, "done send %d", brl_num);
  }

  // If there is any keyboard events
  if (__keyboard.getKeys())
  {
    bool any_brl_key_active = false;
    // Iterate all the active keys
    for (int i = 0; i < LIST_MAX; i++)
    {
      Key key = __keyboard.key[i];
      // If char is in 1 - 6 range inclusive
      if (key.kchar > 48 && key.kchar < 55 && (key.kstate == PRESSED || key.kstate == HOLD))
      {
        any_brl_key_active = true;
        if (braille_key_state == BrailleKeyState::IDLE)
        {
          braille_key_state = BrailleKeyState::PRESSED;
          send_key_time = cur_milli + WAIT_DURATION;
        }
      }

      // Backspace
      if (key.kstate == PRESSED)
      {
        ESP_LOGI(TAG, "Pressed %c", key.kchar);
        press_key(key.kchar);
      }
    }

    if (!any_brl_key_active && braille_key_state == BrailleKeyState::KEYSENT)
    {
      braille_key_state = BrailleKeyState::IDLE;
    }
  }
}