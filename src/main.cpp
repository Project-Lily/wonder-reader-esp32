#include <Arduino.h>
#include <liblouis.h>

void setup() {
  // put your setup code here, to run once:
  widechar inbuf[15] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
  int inbuflen = 15;
  widechar outbuf[15];
  int outbuflen = 15;
  const char *table_list = {"en-us-g1.ctb"};
  widechar outbackbuf[15];
  widechar outbuftranslated[15];
  lou_dotsToChar(table_list, outbuftranslated, outbackbuf, outbuflen, 0);
  printf("Translated string back from dot: ");
  for (int i = 0; i < outbuflen; i++)
    Serial.print(outbackbuf[i]);
}

void loop() {
  // put your main code here, to run repeatedly:
}