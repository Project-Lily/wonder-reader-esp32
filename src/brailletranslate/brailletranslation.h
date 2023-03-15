#ifndef BRAILLETRANSLATION_H
#define BRAILLETRANSLATION_H
#include <string>
namespace wonder {
    void brailleTranslation(std::string, int offset, int maxlength, uint8_t *buffer);
}
#endif