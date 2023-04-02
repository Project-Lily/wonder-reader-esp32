#include "sound.h"
#include "Audio.h"
#include <string>
#include <algorithm>
#include "freertos/task.h"
#include "freertos/semphr.h"

#define I2S_DOUT      22
#define I2S_BCLK      26
#define I2S_LRC       25

static const char *TAG = "sound";

struct SoundTaskParam {
  std::string text;
};

bool should_play_text = false;
std::string text_to_play = "";

Audio audio;
SemaphoreHandle_t play_sound_semaphore;
SoundTaskParam sound_task_param;

void wonder::init_sound() {
  // Init audio
  play_sound_semaphore = xSemaphoreCreateMutex();
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21);
  xSemaphoreGive(play_sound_semaphore);
}

void wonder::loop_sound() {
  xSemaphoreTake(play_sound_semaphore, portMAX_DELAY);
  if (should_play_text) {
    should_play_text = false;
    audio.connecttohost(text_to_play.c_str());
  }
  audio.loop();
  xSemaphoreGive(play_sound_semaphore);
}

void create_host_sound_task(void *pvParameters) {
  ESP_LOGI(TAG, "Sound task created");
  char text[sound_task_param.text.size() + 1];
  strcpy(text, sound_task_param.text.c_str());
  audio.connecttohost(text);
  ESP_LOGI(TAG, "Sound task done");
  xSemaphoreGive(play_sound_semaphore);
}

void wonder::play_text(std::string text) {
  std::string text_temp = text;
  std::replace(text_temp.begin(), text_temp.end(), ' ', '+');

  std::string builder = "https://lilly.arichernando.com/flask/text2speech?text=%22";
  builder.append(text_temp);
  builder.append("%22&lang=en-AU&voice-code=en-AU-News-G");
  sound_task_param.text = builder;
  text_to_play = builder;
  should_play_text = true;
  // audio.connecttohost(builder.c_str());
  // ESP_LOGI(TAG, "Will create sound task: %s", sound_task_param.text.c_str());
  // if (xSemaphoreTake(play_sound_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
  //   xTaskCreate(create_host_sound_task, "Sound task", 4096, NULL, 1, NULL);
  // }
}