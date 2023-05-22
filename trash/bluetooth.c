// :(

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

#define PROFILE_COUNT 1
#define PROFILE_A 0

#define GATTS_SERVICE_A_UUID 0x13
#define GATTS_CHAR_A_UUID 0xF2AB
#define GATTS_DESCR_A_UUID 0x5324

#define adv_config_flag 1 << 1
#define scan_resp_config_flag 1 << 2
static uint8_t adv_config_done = 0;

// We need to define this
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

// Where we store our GATT server profiles
static struct gatts_profile_inst gl_profile_tab[PROFILE_COUNT] = {
  [PROFILE_A] = {
    .gatts_cb = gatts_profile_a_event_handler,
    .gatts_if = ESP_GATT_IF_NONE,
  },
};

static uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

static esp_gatt_char_prop_t a_property = 0;

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
uint8_t char1_str[] = {0x11,0x22,0x33};
esp_attr_value_t gatts_char1_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

static esp_ble_adv_data_t adv_data = {
  .set_scan_rsp = false,
  .include_name = true,
  .include_txpower = false,
  .min_interval = 0x0006,
  .max_interval = 0x0010,
  .appearance = 0,
  .manufacturer_len = 0,
  .p_manufacturer_data = NULL,
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = sizeof(adv_service_uuid128),
  .p_service_uuid = adv_service_uuid128,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_data_t scan_resp_data = {
  .set_scan_rsp = true, // The diff
  .include_name = true,
  .include_txpower = true, // And this
  // .min_interval = 0x0006,
  // .max_interval = 0x0010,
  .appearance = 0,
  .manufacturer_len = 0,
  .p_manufacturer_data = NULL,
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = sizeof(adv_service_uuid128),
  .p_service_uuid = adv_service_uuid128,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
  .adv_int_min = 0x20,
  .adv_int_max = 0x40,
  .adv_type = ADV_TYPE_IND,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
  switch(event) {
    case ESP_GATTS_REG_EVT:
      ESP_LOGI(GATTS_TAG, "Register app event, status: %d, app_id: %d", param->reg.status, param->reg.app_id);
      gl_profile_tab[PROFILE_A].service_id.is_primary = true;
      gl_profile_tab[PROFILE_A].service_id.id.inst_id = 0;
      gl_profile_tab[PROFILE_A].service_id.id.uuid.len = ESP_UUID_LEN_16;
      gl_profile_tab[PROFILE_A].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_A_UUID;

      esp_err_t err;
      if ((err = esp_ble_gap_set_device_name("Wonder Reader"))) {
        ESP_LOGE(GATTS_TAG, "Error setting device name %s", esp_err_to_name(err));
      }

      // Set advertisement data
      if ((err =esp_ble_gap_config_adv_data(&adv_data))) {
        ESP_LOGE(GATTS_TAG, "Failed setting advertisement data %s", esp_err_to_name(err));
      }
      adv_config_done |= adv_config_flag;

      // Set scan response data
      if ((err = esp_ble_gap_config_adv_data(&scan_resp_data))) {
        ESP_LOGE(GATTS_TAG, "Failed setting scan response data %s", esp_err_to_name(err));
      }
      adv_config_done |= scan_resp_config_flag;

      if ((err = esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A].service_id, 4))) {
        ESP_LOGE(GATTS_TAG, "Failed creating service %s", esp_err_to_name(err));
      }
      break;
    case ESP_GATTS_CREATE_EVT:
      ESP_LOGI(GATTS_TAG, "Create service event, status %d, handle %d", param->create.status, param->create.service_handle);
      gl_profile_tab[PROFILE_A].service_handle = param->create.service_handle;
      gl_profile_tab[PROFILE_A].char_uuid.len = ESP_UUID_LEN_16;
      gl_profile_tab[PROFILE_A].char_uuid.uuid.uuid16 = GATTS_CHAR_A_UUID;

      esp_err_t err;
      if ((err = esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A].service_handle))) {
        ESP_LOGE(GATTS_TAG, "Error starting service %s", esp_err_to_name(err));
      }

      a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
      if ((err = esp_ble_gatts_add_char(
        gl_profile_tab[PROFILE_A].service_handle,
        &gl_profile_tab[PROFILE_A].char_uuid,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        a_property,
        &gatts_char1_val,
        NULL
      ))) {
        ESP_LOGE(GATTS_TAG, "Error adding characteristic %s", esp_err_to_name(err));
      }
      break;
    case ESP_GATTS_ADD_CHAR_EVT:
      uint16_t length = 0;
      const uint8_t *prf_char;
      ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
        param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);  
        gl_profile_tab[PROFILE_A].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_A].descr_uuid.len = ESP_UUID_LEN_16;  
        gl_profile_tab[PROFILE_A].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;  
        esp_err_t err = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char); 
      if (err == ESP_FAIL) {
        ESP_LOGE(GATTS_TAG, "Characteristic handle error %s", __func__);
      }
      ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
      for(int i = 0; i < length; i++){
          ESP_LOGI(GATTS_TAG, "prf_char[%x] = %x\n",i,prf_char[i]);
      }

      // Add descriptor
      if ((err = esp_ble_gatts_add_char_descr(
        gl_profile_tab[PROFILE_A].service_handle,
        &gl_profile_tab[PROFILE_A].descr_uuid,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        NULL,
        NULL
      ))) {
        ESP_LOGE(GATTS_TAG, "Error adding descriptor %s", esp_err_to_name(err));
      }
      break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
      ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d",
        param->add_char.status, param->add_char.attr_handle,  
        param->add_char.service_handle);
      break;
    case ESP_GATTS_CONNECT_EVT:
      // When a client has connected to the GATT server
      esp_ble_conn_update_params_t conn_params = {0};
      memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
      conn_params.latency = 0;
      conn_params.max_int = 0x30; // 0x30*1.25ms = 40ms
      conn_params.min_int = 0x10; // 0x10*1.25ms = 20ms
      conn_params.timeout = 400; // 400 * 10ms = 4000ms
      ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x",  
             param->connect.conn_id,  
             param->connect.remote_bda[0],  
             param->connect.remote_bda[1],  
             param->connect.remote_bda[2],  
             param->connect.remote_bda[3],  
             param->connect.remote_bda[4],  
             param->connect.remote_bda[5]);
      gl_profile_tab[PROFILE_A].conn_id = param->connect.conn_id;
      //start sent the update connection parameters to the peer device.
      esp_ble_gap_update_conn_params(&conn_params);
      break;
    case ESP_GATTS_READ_EVT:
      ESP_LOGI(
        GATTS_TAG, "GATT Read event, conn_id %d, trans_id%d, handle %d",
        param->read.conn_id, param->read.trans_id, param->read.handle);
      esp_gatt_rsp_t rsp;
      memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
      rsp.attr_value.handle = param->read.handle;
      rsp.attr_value.len = 4;
      rsp.attr_value.value[0] = 0xde;
      rsp.attr_value.value[1] = 0xed;
      rsp.attr_value.value[2] = 0xbe;
      rsp.attr_value.value[3] = 0xef;
      esp_ble_gatts_send_response(
        gatts_if,
        param->read.conn_id,
        param->read.trans_id,
        ESP_GATT_OK,
        &rsp
      );
      break;
    case ESP_GATTS_WRITE_EVT:
      ESP_LOGI(GATTS_TAG, "GATT Write event conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
      if (!param->write.is_prep) {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);

        if (gl_profile_tab[PROFILE_A].descr_handle == param->write.handle && param->write.len == 2) {
          uint16_t descr_value= param->write.value[1]<<8 | param->write.value[0];
          if (descr_value == 0x1) {
            if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
              ESP_LOGI(GATTS_TAG, "Notify enable");
              uint8_t
            }
          }
        }
      }
      break;
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
  if (event == ESP_GATTS_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
    } else {
      ESP_LOGE(GATTS_TAG, "Register app failed, app_id: %d, status: %d", param->reg.app_id, param->reg.status);
      return;
    }
  }

  do {
    for (int i = 0; i < PROFILE_COUNT; i++) {
      if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gl_profile_tab[i].gatts_if) {
        if (gl_profile_tab[i].gatts_cb) {
          gl_profile_tab[i].gatts_cb(event, gatts_if, param);
        }
      }
    }
  } while (0);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  switch(event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
      adv_config_done &= (~adv_config_flag);
      if (adv_config_done == 0) {
        ESP_LOGI(GAP_TAG, "Start advertising from scan param set");
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
      adv_config_done &= (~scan_resp_config_flag);
      if (adv_config_done == 0) {
        ESP_LOGI(GAP_TAG, "Start advertising from scan response set");
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(GAP_TAG, "Error starting advertising: %d", param->adv_start_cmpl.status);
      }
      break;
  }
}

void wonder::init_bt() {
  esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_err_t err;
  if ((err = esp_bt_controller_init(&cfg))) {
    ESP_LOGE(TAG, "Error initializing bluetooth controller %s", esp_err_to_name(err));
  }

  // if ((err = esp_bt_controller_enable(ESP_BT_MODE_BLE))) {
  if ((err = esp_bt_controller_enable(ESP_BT_MODE_BTDM))) {
    ESP_LOGE(TAG, "Error enabling bluetooth mode BLE %s", esp_err_to_name(err));
  }

  if ((err = esp_bluedroid_init())) {
    ESP_LOGE(TAG, "Error initializing bluedroid %s", esp_err_to_name(err));
  }

  if ((err = esp_bluedroid_enable())) {
    ESP_LOGE(TAG, "Error enabling bluedroid %s", esp_err_to_name(err));
  }

  esp_ble_gap_set_device_name("Wonder Reader");
  // esp_ble_gap_set_appearance(ESP_BLE_APPEARANCE_GENERIC_TAG);
}