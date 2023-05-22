#ifndef __BLUETOOTH_H_
#define __BLUETOOTH_H_

#include <stddef.h>

namespace wonder {
  void init_bt();
  void send_answer(char* answer, size_t len);
}

#endif