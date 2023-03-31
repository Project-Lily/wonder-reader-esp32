#ifndef __AUDIO_H_
#define __AUDIO_H_
#include <string>

namespace wonder {
  void init_sound();
  void loop_sound();
  void play_text(std::string text);
}

#endif