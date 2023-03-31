#ifndef BRAILLETRANSLATION_H
#define BRAILLETRANSLATION_H
#include <string>

namespace wonder {
    size_t text_to_braille(std::string, int offset, int maxlength, uint8_t *buffer);
    std::string braille_to_text(uint8_t *buffer, size_t size);
}
#endif