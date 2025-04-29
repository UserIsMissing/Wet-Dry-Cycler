#include <Arduino.h>


void setup() {
  Serial.begin(115200);      // native USB-CDC (OTG)
  Serial0.begin(115200);     // CP2102 UART
  Serial0.println("Hello over the programming port!");
}


void loop() {

  Serial0.println("Hello, World!");
}
