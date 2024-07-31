#include <Wire.h>
#include <SerLCD.h>

SerLCD lcd;

void setup() {
  Wire.begin();
  lcd.begin(Wire);
  Wire.setClock(400000);
  lcd.clear();
  lcd.cursor();
  delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.setCursor(0, 0);
  lcd.print("20.45MPH");
  lcd.setCursor(9, 0);
  lcd.print("RPM:1500");
  lcd.setCursor(0, 1);
  lcd.print("BT:36C");
  lcd.setCursor(8, 1);
  lcd.print("MC:40C");
  lcd.setCursor(17, 1);
  lcd.print("#13");
  lcd.setCursor(0, 2);
  lcd.print("12A");
  lcd.setCursor(5, 2);
  lcd.print("3.3V");
  lcd.setCursor(14, 2);
  lcd.print("3.625V");
  lcd.setCursor(0, 3);
  lcd.print("0.0KWH");
  lcd.setCursor(9, 3);
  lcd.print("TRIP:123.2");
}
