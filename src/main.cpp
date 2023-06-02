#include <Arduino.h>
#include <SPIFFS.h>
#include "brailletranslate/brailletranslation.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/task.h"
#include "wonderconfig.h"
#include "network.h"
#include "bluetooth/bluetooth.h"
#include "control/braillecontrol.h"
#include "control/keyboard.h"
#include "sound.h"

static const char* TAG = "main";

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

  // disableCore1WDT();

  // Initialize main loop
  // This main loop is initialized so that we can register event handlers.
  // Without this, event handler will go: Bruh
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wonder::init_board();
  wonder::init_network();
  wonder::init_bt();
  wonder::init_motors();
  wonder::init_sound();

  xTaskCreatePinnedToCore(wonder::braille_task, "Braille Task", 2048, NULL, configMAX_PRIORITIES - 1, NULL, 1);

  int taskCount = uxTaskGetNumberOfTasks();
  ESP_LOGI(TAG, "Systems initialized, %d tasks running", taskCount);
  TaskStatus_t* taskStatusArray = (TaskStatus_t*)malloc(sizeof(TaskStatus_t) * taskCount);

  wonder::display_text("Hello!");
}

// long nextSend = 0;
// long interval = 5000;
void loop() {
  wonder::loop_sound();
  // long milli = millis();
  // if (milli > nextSend) {
  //   char buf[128];
  //   size_t len = sprintf(buf, "{\"event\": \"answer\", \"data\": {\"time\": %d}}", esp_timer_get_time() / 1000);
  //   wonder::send_answer(buf, len);
  //   nextSend = milli + interval;
  // }
  // wonder::process_events();
}