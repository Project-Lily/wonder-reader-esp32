#include <iostream>
#include <Arduino.h>
#include <liblouis.h>
#include <string>
void brailleTranslation(std::string input, int offset, int maxlength) {
    const char* table_list = {"en-us-g1.ctb"};
    widechar outbuf[maxlength];
    int inbuflen = input.size();
    int outbuflen = maxlength;

    for (int i = 0; i < inbuflen; i++) {
        outbuf[i] = input[i];
    }

    lou_translateString(table_list, outbuf, &inbuflen, outbuf, &outbuflen, NULL, NULL, 0);
    widechar outbuftranslated[15];
    lou_charToDots(table_list, outbuf, outbuftranslated, outbuflen, 0);

    int binaryNum[32]; 
    std::string binaryStr;

    for (int i = 0; i < outbuflen; i++) {
        if (outbuftranslated[i + offset] != 0) {
            int num = outbuftranslated[i + offset] ^ 32768;
            int j = 0;
            for (; num > 0;) {
                binaryNum[j++] = num % 2;
                num /= 2;
            }
            for (int k = j - 1; k >= 0; k--) {
                std::cout << binaryNum[k];
            }
        }
    }
}
