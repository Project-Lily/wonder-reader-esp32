#include <Arduino.h>
#include <liblouis.h>
#include "brailletranslate.h"
void setup() {
  std::string input = "Hello World!";
  wonder::brailleTranslation(input, 0, 15);
}

void loop() {
  // put your main code here, to run repeatedly:
}