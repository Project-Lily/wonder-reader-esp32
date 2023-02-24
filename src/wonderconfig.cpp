#include "wonderconfig.h"
#include "Preferences.h"
#include "esp_log.h"

#define CONFIG_NAMESPACE "wonderconf"

static const char* TAG = "config";

const char* wonder::get_config_string(WonderCfg cfg_enum) {
  return _cfg_string[cfg_enum];
}

bool wonder::get_preferences(Preferences* pref) {
  return pref->begin(CONFIG_NAMESPACE);
}

bool wonder::init_configuration() {
  Preferences pref;
  if (!get_preferences(&pref)) {
    return false;
  }

  // If configuration has not been init, then load all default to config
  if (!pref.getBool("conf_init", false)) {
    pref.putDouble(get_config_string(X_STEP_PER_MM), DEF_X_STEP_PER_MM);
    pref.putDouble(get_config_string(Y_STEP_PER_MM), DEF_Y_STEP_PER_MM);
    pref.putDouble(get_config_string(MAIN_LIMIT_TO_FIRST_CELL), DEF_MAIN_LIMIT_TO_FIRST_CELL);
    pref.putBool("conf_init", true);
  }
}

void wonder::print_preferences() {
  Preferences pref;
  if (!get_preferences(&pref)) {
    ESP_LOGE(TAG, "Error printing preferences");
    return;
  }

  ESP_LOGD(TAG, "Wonder Reader Config:");
  ESP_LOGD(TAG, "%s: %d", get_config_string(X_STEP_PER_MM), pref.getDouble(get_config_string(X_STEP_PER_MM)));
  ESP_LOGD(TAG, "%s: %d", get_config_string(Y_STEP_PER_MM), pref.getDouble(get_config_string(Y_STEP_PER_MM)));
  ESP_LOGD(TAG, "%s: %d", get_config_string(MAIN_LIMIT_TO_FIRST_CELL), pref.getDouble(get_config_string(MAIN_LIMIT_TO_FIRST_CELL)));
}