// 🔧 Enhanced CAN Debug Logger
#include <Canbus.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include <SerLCD.h>
#include <math.h>

#define RXPIN 4
#define TXPIN 5
#define GPSBAUD 4800
#define SD_CS 9

#define CAN_ID_VOLTAGE_CURRENT 0x03B
#define CAN_ID_SOC 0x1A0
#define CAN_ID_MAX_CELL_V 0x1B1

SoftwareSerial uart_gps(RXPIN, TXPIN);
TinyGPS gps;
SerLCD lcd;

void setup() {
  delay(2000);
  Serial.begin(9600);
  uart_gps.begin(GPSBAUD);
  Wire.begin();
  lcd.begin(Wire);
  lcd.setAddress(0x72);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("🔧 CAN Debug Init...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card init failed!");
    lcd.setCursor(0, 1); lcd.print("SD FAIL ❌");
  } else {
    Serial.println("SD card initialized!");
    lcd.setCursor(0, 1); lcd.print("SD OK ✅");
  }

  if (Canbus.init(CANSPEED_125)) {
    Serial.println("✅ CAN Init ok");
    lcd.setCursor(0, 2); lcd.print("CAN OK ✅");
  } else {
    Serial.println("❌ CAN Init failed");
    lcd.setCursor(0, 2); lcd.print("CAN FAIL ❌");
  }

  lcd.setCursor(0, 3);
  lcd.print("🔍 Sniffing...");
}

void loop() {
  tCAN message;
  if (mcp2515_check_message() && mcp2515_get_message(&message)) {
    Serial.print("\n📡 CAN ID: 0x"); Serial.print(message.id, HEX);
    Serial.print(" | Data: ");
    for (int i = 0; i < message.header.length; i++) {
      Serial.print(message.data[i], HEX); Serial.print(" ");
    }
    Serial.println();

    // Special debug: SOC
    if (message.id == CAN_ID_SOC) {
      Serial.print("[DEBUG] SOC ID 0x1A0 Bytes: ");
      for (int i = 0; i < message.header.length; i++) {
        Serial.print(message.data[i], HEX); Serial.print(" ");
      }
      Serial.println();
    }

    // Special debug: Voltage/Current
    if (message.id == CAN_ID_VOLTAGE_CURRENT) {
      Serial.print("[DEBUG] V/I ID 0x03B Bytes: ");
      for (int i = 0; i < message.header.length; i++) {
        Serial.print(message.data[i], HEX); Serial.print(" ");
      }
      Serial.println();

      int current_raw = message.data[0] * 256 + message.data[1];
      float current = current_raw / 10.0;
      int voltage_raw = message.data[2] * 256 + message.data[3];
      float voltage = voltage_raw / 10.0;
      Serial.print("Decoded Current: "); Serial.print(current); Serial.print(" A\n");
      Serial.print("Decoded Voltage: "); Serial.print(voltage); Serial.println(" V");
    }

    // Special debug: Highest Cell Voltage
    if (message.id == CAN_ID_MAX_CELL_V) {
      Serial.print("[DEBUG] Max Cell Voltage ID 0x1B1 Bytes: ");
      for (int i = 0; i < message.header.length; i++) {
        Serial.print(message.data[i], HEX); Serial.print(" ");
      }
      Serial.println();
      int highCell_raw = message.data[0] * 256 + message.data[1];
      float highCellV = highCell_raw / 1000.0;
      Serial.print("High Cell Voltage: "); Serial.print(highCellV); Serial.println(" V");
    }
  }

  delay(500);
}
