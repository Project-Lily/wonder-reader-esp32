#ifndef __WONDERCONFIG_H_
#define __WONDERCONFIG_H_

#include "Preferences.h"

/** 
 * Stepper stepper / mm config
 */
#define DEF_X_STEP_PER_MM 20.0
#define DEF_Y_STEP_PER_MM 10.0

/**
 * Range bweteen when the stepper motor hits the limit switch to the first
 * braille cell in mm
 */
#define DEF_MAIN_LIMIT_TO_FIRST_CELL 15.0

/**
 * Preprocessor trick for string enum thanks to
 * Mr. Terrence M. https://stackoverflow.com/a/10966395/12709867
 */
#define FOREACH_CONFIG(CONF) \
        CONF(X_STEP_PER_MM)   \
        CONF(Y_STEP_PER_MM)  \
        CONF(MAIN_LIMIT_TO_FIRST_CELL)   \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

namespace wonder {
  typedef enum {
      FOREACH_CONFIG(GENERATE_ENUM)
  } WonderCfg;

  static const char* _cfg_string[] = {
      FOREACH_CONFIG(GENERATE_STRING)
  };

  // Get the config string from the macro enum
  const char* get_config_string(WonderCfg cfg_enum);

  // Initialize the configuration from defaults. Needed to be only run once
  // in the beginning
  bool init_configuration();

  // Init the supplied preferences object
  bool get_preferences(Preferences* pref);

  // Print all preferences
  void print_preferences();
}

#endif