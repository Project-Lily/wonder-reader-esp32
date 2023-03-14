#include <Arduino.h>
#include <liblouis.h>
#include <SPIFFS.h>
#include "wonderconfig.h"
#include "control/braillecontrol.h"
#include <mdns.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_netif.h"

static const char* TAG = "main";

void init_mdns() {
  const char* hostname = "wreader-1";

  // Start mDNS
  ESP_ERROR_CHECK(mdns_init());
  ESP_ERROR_CHECK(mdns_hostname_set(hostname));
  ESP_ERROR_CHECK(mdns_instance_name_set("Wonder Reader 1"));
  ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));

  ESP_LOGI(TAG, "mDNS Initialized. Hostname: %s", hostname);
}

void wifi_event_handler(void* arg, esp_event_base_t event_base,
  int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    ESP_LOGI(TAG, "Connected to Wi-Fi!");
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    ESP_LOGI(TAG, "Disconnected. Attempting to reconnect to Wi-Fi");
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* ip_event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "Got IP Address:" IPSTR, IP2STR(&ip_event->ip_info.ip));
  }
}

void init_network() {
  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_create_default_wifi_sta();

  // Register event handler
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  
  wifi_config_t wifi_config = {
    .sta = {
      {.ssid = "SSID"},
      {.password = "PASSWORD"}
    },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "Network initialized");
}

void setup() {
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

  init_network();
  init_mdns();

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