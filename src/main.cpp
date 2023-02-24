#include <Arduino.h>
#include <liblouis.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <stdio.h>
#include <SPIFFS.h>
#include "esp_log.h"

const char* TAG = "main";

void setup() {
  ESP_LOGI(TAG, "Wonder Reader Hello!");

  // put your setup code here, to run once:
  if(!SPIFFS.begin(true)){
    ESP_LOGE(TAG, "An Error has occurred while mounting SPIFFS");
    return;
  }

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