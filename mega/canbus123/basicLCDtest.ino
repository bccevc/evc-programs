#include <Wire.h>
#include <SerLCD.h>

SerLCD lcd; // create an lcd object

void setup() {
  delay(2000); // Let board + LCD stabilize
  Wire.begin();
  lcd.begin(Wire); // Initialize LCD with I2C
  lcd.setBacklight(255, 255, 255); // Full white backlight (RGB)

  lcd.clear(); // Clear any junk
  lcd.setCursor(0, 0);
  lcd.print("✅ LCD Test OK");
  lcd.setCursor(0, 1);
  lcd.print("Line 2 is here!");

  Serial.begin(9600);
  Serial.println("LCD test running...");
}

void loop() {
  // nothing here — just static test
}
