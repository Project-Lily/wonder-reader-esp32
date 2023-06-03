#ifndef __WONDERCONFIG_H_
#define __WONDERCONFIG_H_

#include "Preferences.h"

// #define TEST_NO_MOVE_MODE

/** 
 * Stepper stepper / mm config
 */
#define DEF_XSTEP_PER_MM 20.0
#define DEF_YSTEP_PER_MM 10.0

/**
 * Range bweteen when the stepper motor hits the limit switch to the first
 * braille cell in mm
 */
#define DEF_LIMIT_TO_CELL 15.0

#define DEF_X_SPEED 3000.0
#define DEF_X_ACCEL 6000.0
#define DEF_X_HOME_SPEED 1500.0

#define DEF_Y_HOME_SPEED 4000.0
#define DEF_Y_SPEED 15000.0
#define DEF_Y_ACCEL 12000.0

/**
 * Preprocessor trick for string enum thanks to
 * Mr. Terrence M. https://stackoverflow.com/a/10966395/12709867
 */
#define FOREACH_CONFIG(CONF) \
        CONF(XSTEP_PER_MM)   \
        CONF(YSTEP_PER_MM)  \
        CONF(LIMIT_TO_CELL)   \
        CONF(X_HOME_SPEED)   \
        CONF(X_SPEED)   \
        CONF(X_ACCEL)   \
        CONF(Y_HOME_SPEED)   \
        CONF(Y_SPEED)   \
        CONF(Y_ACCEL)   \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

namespace wonder {
  void _reset_configuration(Preferences* pref);

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

  // Reset configuration
  bool reset_configuration();

  // Init the supplied preferences object
  bool get_configuration(Preferences* pref);

  // Print all preferences
  void print_configuration();
}

#endif