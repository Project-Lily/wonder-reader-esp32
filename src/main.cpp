#include <Arduino.h>
#include <liblouis.h>
#include "SPIFFS.h"

void decimalToBinary(widechar num) {   
    if (num == 0) {
        Serial.print("0");
        return;
    }
   
   // Stores binary representation of number.
   int binaryNum[32]; // Assuming 32 bit integer.
   int i=0;
   
   for ( ;num > 0; ){
      binaryNum[i++] = num % 2;
      num /= 2;
   }
   
   // Printing array in reverse order.
   for (int j = i-1; j >= 0; j--)
      Serial.print(binaryNum[j]);
}
void brailleTranslation(std::string input, int offset, int maxlength){
  widechar arr[input.size()];
  for (int i = 0; i < input.size(); i++) {
      arr[i] = input[i];
  }
  const char *table_list = {"en-us-g1.ctb"};
  widechar outbuf[offset];
  int inbuflen = maxlength;
  int outbuflen = maxlength;
  
  lou_translateString(table_list, arr, &inbuflen, outbuf, &outbuflen, NULL, NULL, 0);
  widechar outbuftranslated[15];
  Serial.print("Translated string: ");
  for (int i = 0; i < outbuflen; i++)
    Serial.print(outbuf[i] + '\n');

  lou_charToDots(table_list, outbuf, outbuftranslated, outbuflen, 0);
  Serial.print("Translated string to dot: ");
  for (int i = 0; i < outbuflen; i++) {
    decimalToBinary(outbuftranslated[i + offset] ^ 32768);
    Serial.print(" ");
  }
    // Serial.print("%d ", outbuf[i] ^ 32768);
  Serial.print("\n");
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Hello World!");
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();
 
  while(file){
 
      Serial.print("FILE: ");
      Serial.println(file.name());
 
      file = root.openNextFile();
  }
  std::string input = "Hello World!";
  brailleTranslation(input, 0, 15);
}

void loop() {
  // put your main code here, to run repeatedly:
}