#include "wonderconfig.h"
#include "Preferences.h"
#include "esp_log.h"

#define CONFIG_NAMESPACE "wonderconf"

static const char* TAG = "config";

const char* wonder::get_config_string(WonderCfg cfg_enum) {
  return _cfg_string[cfg_enum];
}

void wonder::_reset_configuration(Preferences* pref) {
  pref->putDouble(get_config_string(XSTEP_PER_MM), DEF_XSTEP_PER_MM);
  pref->putDouble(get_config_string(YSTEP_PER_MM), DEF_YSTEP_PER_MM);
  pref->putDouble(get_config_string(LIMIT_TO_CELL), DEF_LIMIT_TO_CELL);
  pref->putDouble(get_config_string(X_SPEED), DEF_X_SPEED);
  pref->putDouble(get_config_string(X_ACCEL), DEF_X_ACCEL);
  pref->putDouble(get_config_string(X_HOME_SPEED), DEF_X_HOME_SPEED);
  pref->putDouble(get_config_string(Y_SPEED), DEF_Y_SPEED);
  pref->putDouble(get_config_string(Y_ACCEL), DEF_Y_ACCEL);
  pref->putBool("conf_init", true);
}

bool wonder::get_configuration(Preferences* pref) {
  return pref->begin(CONFIG_NAMESPACE);
}

bool wonder::init_configuration() {
  Preferences pref;
  if (!get_configuration(&pref)) {
    return false;
  }

  // If configuration has not been init, then load all default to config
  if (!pref.getBool("conf_init", false)) {
    _reset_configuration(&pref);
    ESP_LOGI(TAG, "Configuration initialized");
  } else {
    ESP_LOGI(TAG, "Configuration detected");
  }

  return true;
}

bool wonder::reset_configuration() {
  Preferences pref;
  if (!get_configuration(&pref)) {
    return false;
  }

  _reset_configuration(&pref);
  return true;
}

void wonder::print_configuration() {
  Preferences pref;
  if (!get_configuration(&pref)) {
    ESP_LOGE(TAG, "Error printing preferences");
    return;
  }

  ESP_LOGI(TAG, "Wonder Reader Config:");
  ESP_LOGI(TAG, "%s: %f", get_config_string(XSTEP_PER_MM), pref.getDouble(get_config_string(XSTEP_PER_MM)));
  ESP_LOGI(TAG, "%s: %f", get_config_string(YSTEP_PER_MM), pref.getDouble(get_config_string(YSTEP_PER_MM)));
  ESP_LOGI(TAG, "%s: %f", get_config_string(LIMIT_TO_CELL), pref.getDouble(get_config_string(LIMIT_TO_CELL)));
  ESP_LOGI(TAG, "%s: %f", get_config_string(X_SPEED), pref.getDouble(get_config_string(X_SPEED)));
  ESP_LOGI(TAG, "%s: %f", get_config_string(X_ACCEL), pref.getDouble(get_config_string(X_ACCEL)));
  ESP_LOGI(TAG, "%s: %f", get_config_string(X_HOME_SPEED), pref.getDouble(get_config_string(X_HOME_SPEED)));
  ESP_LOGI(TAG, "%s: %f", get_config_string(Y_SPEED), pref.getDouble(get_config_string(Y_SPEED)));
  ESP_LOGI(TAG, "%s: %f", get_config_string(Y_ACCEL), pref.getDouble(get_config_string(Y_ACCEL)));
}