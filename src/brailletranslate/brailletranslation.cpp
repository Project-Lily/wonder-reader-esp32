#include "brailletranslation.h"
#include "esp_log.h"
#include <iostream>
#include <Arduino.h>
#include <string>

// Translates simple braille ready character to its binary form
uint8_t charToBraille(uint8_t chr) {
  // If letter is lowercase, uppercase it
  if (chr > 96 && chr < 123) {
    chr ^= 32;
  }
  switch (chr) {
    case '(space)': return 0;
    case '!': return 46;
    case '"': return 16;
    case '#': return 60;
    case '$': return 43;
    case '%': return 41;
    case '&': return 47;
    case '\'': return 4;
    case '(': return 55;
    case ')': return 62;
    case '*': return 33;
    case '+': return 44;
    case ',': return 32;
    case '-': return 36;
    case '.': return 40;
    case '/': return 12;
    case '0': return 52;
    case '1': return 2;
    case '2': return 6;
    case '3': return 18;
    case '4': return 50;
    case '5': return 34;
    case '6': return 22;
    case '7': return 54;
    case '8': return 38;
    case '9': return 20;
    case ':': return 49;
    case ';': return 48;
    case '<': return 35;
    case '=': return 63;
    case '>': return 28;
    case '?': return 57;
    case '@': return 8;
    case 'A': return 1;
    case 'B': return 3;
    case 'C': return 9;
    case 'D': return 25;
    case 'E': return 17;
    case 'F': return 11;
    case 'G': return 27;
    case 'H': return 19;
    case 'I': return 10;
    case 'J': return 26;
    case 'K': return 5;
    case 'L': return 7;
    case 'M': return 13;
    case 'N': return 29;
    case 'O': return 21;
    case 'P': return 15;
    case 'Q': return 31;
    case 'R': return 23;
    case 'S': return 14;
    case 'T': return 30;
    case 'U': return 37;
    case 'V': return 39;
    case 'W': return 58;
    case 'X': return 45;
    case 'Y': return 61;
    case 'Z': return 53;
    case '[': return 42;
    case '\\': return 51;
    case ']': return 59;
    case '^': return 24;
    case '_': return 56;
    default: return 0;
  }
}

void wonder::brailleTranslation(std::string input, int offset, int maxlength, uint8_t *buffer) {
  // First for pass is to create the braille string representation
  std::string result;
  // Iterate the input string
  for (std::string::iterator it = input.begin(); it != input.end(); ++it) {
    char chr = *it;
    // If character is uppercase
    if (chr > 64 && chr < 91) {
      char up[3];
      sprintf(up, ",%c", chr | 32);
      result.append(up);
      continue;
    }
    if (chr > 47 && chr < 58) {
      char num[3];
      sprintf(num, "#%c", chr);
      result.append(num);
      continue;
    }
    // If character is number
    switch(chr) {
      case '"':
        result.push_back('0');
        break;
      case ',':
        result.push_back('1');
        break;
      case ';':
        result.push_back('2');
        break;
      case ':':
        result.push_back('3');
        break;
      case '.':
        result.push_back('4');
        break;
      case '!':
        result.push_back('6');
        break;
      case '(':
      case ')':
        result.push_back('7');
        break;
      case '?':
        result.push_back('8');
        break;
      default:
        result.push_back(chr);
    }
  }

  // Second pass is to get the char
  std::string substr = result.substr(offset, maxlength);
  for (std::string::iterator it = substr.begin(); it != substr.end(); ++it) {
    buffer[it - substr.begin()] = charToBraille(*it);
  }
}
