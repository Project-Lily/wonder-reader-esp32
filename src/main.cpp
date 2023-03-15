#include <Arduino.h>
#include <liblouis.h>
#include "brailletranslate/brailletranslation.h"
#include <SPIFFS.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "wonderconfig.h"
#include "control/braillecontrol.h"
#include "network.h"

static const char* TAG = "main";

void setup() {
  std::string input = "Hello World!";
  wonder::brailleTranslation(input, 0, 15);

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

  // Initialize main loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wonder::init_network();
  wonder::init_mdns();

  // wonder::init_motors();

  widechar inbuf[15] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
  int inbuflen = 15;
  widechar outbuf[15];
  int outbuflen = 15;
  const char *table_list = {"en-us-g1.ctb"};
  widechar outbackbuf[15];
  widechar outbuftranslated[15];
  lou_dotsToChar(table_list, outbuftranslated, outbackbuf, outbuflen, 0);
  ESP_LOGI(TAG, "Translated string back from dot: ");
  for (int i = 0; i < outbuflen; i++){
    ESP_LOGI(TAG, "%d", outbackbuf[i]);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}