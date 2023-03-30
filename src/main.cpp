#include <Arduino.h>
#include <stdio.h>
#include "brailletranslate/brailletranslation.h"
#include <SPIFFS.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "wonderconfig.h"
#include "control/braillecontrol.h"
#include "network.h"
#include "Audio.h"
#include "control/keyboard.h"

static const char* TAG = "main";
Audio audio;
Keypad *keyboard;

#define I2S_DOUT      22
#define I2S_BCLK      26
#define I2S_LRC       25

void setup() {
  esp_log_level_set("*", ESP_LOG_INFO);
  ESP_LOGI(TAG, "Wonder Reader Hello!");

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
  ESP_ERROR_CHECK(ret);

  // Initialize configurtion and print it
  if (!wonder::init_configuration()) {
    ESP_LOGE(TAG, "Error loading configuration");
    return;
  }
  wonder::print_configuration();

  // Initialize spiffs
  if(!SPIFFS.begin(true)){
    ESP_LOGE(TAG, "An Error has occurred while mounting SPIFFS");
    return;
  }

  wonder::init_board();

  // Initialize main loop
  // This main loop is initialized so that we can register event handlers.
  // Without this, event handler will go: Bruh
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wonder::init_network();

  wonder::init_motors();

  // Init audio
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);

  audio.connecttohost("https://lilly.arichernando.com/flask/text2speech?text=%22hello%22&lang=en-AU&voice-code=en-AU-News-G");

  ESP_LOGI(TAG, "Systems initialized");
}

void loop() {
  audio.loop();
  wonder::process_events();
}