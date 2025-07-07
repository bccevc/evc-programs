// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>
 
#define REDLITE 3
#define GREENLITE 5
#define BLUELITE 6
 
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
 
// you can change the overall brightness by range 0 -> 255
int brightness = 255;
 
void setup() {
  // set up the LCD's number of rows and columns: 
  lcd.begin(16, 2);

  // Print a message to the LCD.
  lcd.print("Hello Myko ");
  lcd.setCursor(0,1);
  lcd.print("Hello Prof. Roller");
  
  pinMode(REDLITE, OUTPUT);
  pinMode(GREENLITE, OUTPUT);
  pinMode(BLUELITE, OUTPUT);

  brightness = 100;
}
 
 
void loop() {
  for (int i = 0; i < 255; i++) {
    setBacklight(i, 0, 255-i);
    delay(50);
  }
  for (int i = 0; i < 255; i++) {
    setBacklight(255-i, i, 0);
    delay(40);
  }
  for (int i = 0; i < 255; i++) {
    setBacklight(0, 255-i, i);
    delay(30);
  }
}
 
 
 
void setBacklight(uint8_t r, uint8_t g, uint8_t b) {
  // normalize the red LED - its brighter than the rest!
  r = map(r, 0, 255, 0, 100);
  g = map(g, 0, 255, 0, 150);
 
  r = map(r, 0, 255, 0, brightness);
  g = map(g, 0, 255, 0, brightness);
  b = map(b, 0, 255, 0, brightness);
 
  // common anode so invert!
  r = map(r, 0, 255, 255, 0);
  g = map(g, 0, 255, 255, 0);
  b = map(b, 0, 255, 255, 0);
  Serial.print("R = "); Serial.print(r, DEC);
  Serial.print(" G = "); Serial.print(g, DEC);
  Serial.print(" B = "); Serial.println(b, DEC);
  analogWrite(REDLITE, r);
  analogWrite(GREENLITE, g);
  analogWrite(BLUELITE, b);
}
