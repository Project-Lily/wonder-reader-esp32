#include "network.h"
#include "secret.h"
// #include <mdns.h>
#include "esp_log.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_system.h"
// #include "esp_netif.h"
// #include "esp_http_server.h"
#include "WiFi.h"
#include "esp_event.h"
#include "esp_websocket_client.h"
#include "esp_http_client.h"
#include <ArduinoJson.h>
#include <string>
#include "control/braillecontrol.h"

static const char* TAG = "net";

static esp_websocket_client_handle_t ws_client;

// extern void register_routes(httpd_handle_t *server);

// httpd_handle_t server = NULL;
// httpd_config_t server_conf = HTTPD_DEFAULT_CONFIG();

// void init_mdns() {
//   const char* hostname = "wreader-1";

//   // Start mDNS
//   ESP_ERROR_CHECK(mdns_init());
//   ESP_ERROR_CHECK(mdns_hostname_set(hostname));
//   ESP_ERROR_CHECK(mdns_instance_name_set("Wonder Reader 1"));

//   // Add the service
//   mdns_txt_item_t wonder1_txt[] = {
//     {"protocol_ver", "1"}, // increment this. No need for semantic versioning
//     {"path", "/connect"},
//   };

//   ESP_ERROR_CHECK(mdns_service_add(NULL, "_wonderreader", "_tcp",
//     HTTP_PORT, wonder1_txt, sizeof(wonder1_txt)/sizeof(wonder1_txt[0])));

//   ESP_LOGI(TAG, "mDNS Initialized. Hostname: %s", hostname);
// }

// void wifi_event_handler(void* arg, esp_event_base_t event_base,
//   int32_t event_id, void* event_data) {
//   if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//     ESP_LOGI(TAG, "Connected to Wi-Fi!");
//     esp_wifi_connect();
//     ESP_LOGW(TAG, "Connected to new Wi-Fi!");
//   } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//     ESP_LOGI(TAG, "Disconnected. Attempting to reconnect to Wi-Fi");
//     // stop_server();
//     esp_wifi_connect();
//   } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//     ip_event_got_ip_t* ip_event = (ip_event_got_ip_t*) event_data;
//     // start_server();
//     ESP_LOGW(TAG, "Got IP Address: " IPSTR, IP2STR(&ip_event->ip_info.ip));
    
//   }
// }

// void stop_server() {
//   if (server == NULL) return;

//   if (httpd_stop(&server) == ESP_OK) {
//     server = NULL;
//     ESP_LOGI(TAG, "HTTP Server stopped");
//   } else {
//     ESP_LOGE(TAG, "Error stopping HTTP server");
//   };
// }

// void start_server() {
//   if (server != NULL) return;

//   if (httpd_start(&server, &server_conf) == ESP_OK) {
//     ESP_LOGI(TAG, "Started HTTP server at port %d", HTTP_PORT);
//   } else {
//     ESP_LOGE(TAG, "Error starting HTTP server");
//   };
// }

// void wonder::init_network() {
//   ESP_ERROR_CHECK(esp_netif_init());
//   esp_netif_create_default_wifi_sta();

//   // Register event handler
//   esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
//   esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

//   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//   ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  
//   wifi_config_t wifi_config = {
//     .sta = {
//       {.ssid = WIFI_SSID},
//       {.password = WIFI_PASSWORD}
//     },
//   };

//   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//   ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//   ESP_ERROR_CHECK(esp_wifi_start());

//   init_mdns();

//   // Change server config a bit
//   server_conf.server_port = HTTP_PORT;

//   ESP_LOGI(TAG, "Network initialized");
// }

void wonder::init_network() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  ESP_LOGI(TAG, "Network initialized");
}

bool room_connected = false;

esp_err_t http_event_handler(esp_http_client_event_t *ev) {
  return ESP_OK;
}

void join_room(void *pvParameters) {
  char *id = (char*)pvParameters;
  esp_http_client_config_t conf = {
    .url = "https://lilly.arichernando.com/flask/student/join",
    .method = HTTP_METHOD_POST,
    .event_handler = http_event_handler,
  };

  esp_http_client_handle_t request = esp_http_client_init(&conf);
  esp_http_client_set_header(request, "Content-Type", "application/json");

  std::string data = "{\"id\": \"";
  data.append(id);
  data.append("\", \"name\": \"wonder\"}");

  esp_http_client_set_post_field(request, data.c_str(), data.length());

  esp_err_t err = esp_http_client_perform(request);

  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Successfully joined room");
    room_connected = true;
  } else {
    ESP_LOGE(TAG, "Failed to join room.");
  }

  esp_http_client_cleanup(request);
  vTaskDelete(NULL);
}

static void student_mode_ws_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
  DynamicJsonDocument doc(100);

  switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
      ESP_LOGI(TAG, "Student connected");
      break;
    case WEBSOCKET_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "Student disconnected");
      break;
    case WEBSOCKET_EVENT_DATA:
      ESP_LOGI(TAG, "Received websocket data: %.*s", data->data_len, (char*)data->data_ptr);
      // Parse json
      deserializeJson(doc, (char*)data->data_ptr);
      char *data = doc["data"];
      if (room_connected) {
        std::string text(data);
        wonder::display_text(text);
      } else {
        xTaskCreatePinnedToCore(join_room, "Join Room Task", 2048, data, 1, NULL, 0);
      }
      break;
    case WEBSOCKET_EVENT_ERROR:
      ESP_LOGE(TAG, "Websocket student mode error");
      break;
  }
}

void wonder::init_student_mode() {
  ESP_LOGI(TAG, "Initializing student mode");
  esp_websocket_client_config_t config = {
    .uri = "wss://lilly.arichernando.com/flask",
    .disable_auto_reconnect = false,
    .cert_pem = NULL,
  };

  ws_client = esp_websocket_client_init(&config);
  esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, student_mode_ws_handler, NULL);
  esp_websocket_client_start(ws_client);
  ESP_LOGI(TAG, "Student mode activated");
}