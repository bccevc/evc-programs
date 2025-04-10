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
#define CAN_ID_MIN_CELL_V 0x1B0
#define CAN_ID_MAX_CELL_V 0x1B1
#define CAN_ID_MAX_TEMP 0x1C0
#define CAN_ID_DISCHARGE 0x1D0
#define CAN_ID_CHARGER_SAFE 0x1D1
#define CAN_ID_BALANCING 0x1D2
#define CAN_ID_ERROR_FLAGS 0x1EF
#define CAN_ID_LOW_CELL_ID 0x1E0
#define CAN_ID_HIGH_CELL_ID 0x1E1
#define CAN_ID_THERMISTOR_ID 0x1E2

SoftwareSerial uart_gps(RXPIN, TXPIN);
TinyGPS gps;
SerLCD lcd;
File logCsv;

float latitude, longitude, lastLat = 0, lastLon = 0;
float totalDist = 0.0, energyUsed = 0.0;
unsigned long startTime;
String csvName = "";

String generateFileName(String prefix, String ext) {
  int year; byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  char filename[32];
  sprintf(filename, "%s_%02d%02d%04d_%02d%02d.%s", prefix.c_str(), month, day, year, hour, minute, ext.c_str());
  return String(filename);
}

void printCANMessage(tCAN message) {
  Serial.print("📡 CAN ID: 0x");
  Serial.print(message.id, HEX);
  Serial.print(" | Data: ");
  for (int i = 0; i < message.length; i++) {
    if (message.data[i] < 16) Serial.print("0");
    Serial.print(message.data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void setup() {
  delay(2000);
  Serial.begin(9600);
  uart_gps.begin(GPSBAUD);
  Wire.begin();

  lcd.begin(Wire);
  lcd.setAddress(0x72);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("🔌 Booting Logger...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card init failed!");
    lcd.setCursor(0, 1);
    lcd.print("SD FAIL ❌");
    return;
  }
  Serial.println("SD card initialized!");
  lcd.setCursor(0, 1);
  lcd.print("SD OK ✅");

  if (Canbus.init(CANSPEED_125)) {
    Serial.println("CAN Init ok");
    lcd.setCursor(0, 2);
    lcd.print("CAN OK ✅");
  } else {
    Serial.println("Can't init CAN");
    lcd.setCursor(0, 2);
    lcd.print("CAN FAIL ❌");
  }

  int year = 0; byte month, day, hour, minute, second, hundredths;
  while (year == 0) {
    while (uart_gps.available()) gps.encode(uart_gps.read());
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  }

  lcd.setCursor(0, 3);
  lcd.print("GPS OK ✅");

  startTime = millis();
  csvName = generateFileName("log", "csv");

  logCsv = SD.open(csvName, FILE_WRITE);
  if (logCsv) {
    logCsv.println("Time(ms),RawVoltage,RawCurrent,RawSOC");
    logCsv.close();
  }

  delay(3000);
  lcd.clear();
}

void loop() {
  unsigned long now = millis();
  if (uart_gps.available()) {
    gps.encode(uart_gps.read());
    gps.f_get_position(&latitude, &longitude);
  }

  tCAN message;
  if (mcp2515_check_message() && mcp2515_get_message(&message)) {
    // Print all CAN messages
    printCANMessage(message);

    if (message.id == CAN_ID_VOLTAGE_CURRENT) {
      // Decode current as signed 16-bit (big endian)
      int16_t rawCurrent = (message.data[0] << 8) | message.data[1];
      float current = rawCurrent / 10.0;

      // Voltage from byte 3
      float voltage = message.data[3];

      Serial.print("🔋 Voltage: "); Serial.print(voltage);
      Serial.print(" V | ⚡ Current: "); Serial.print(current); Serial.println(" A");

      // Log to CSV
      logCsv = SD.open(csvName, FILE_WRITE);
      if (logCsv) {
        logCsv.print(now); logCsv.print(",");
        logCsv.print(voltage); logCsv.print(",");
        logCsv.print(current); logCsv.print(",");
        logCsv.println("?");
        logCsv.close();
      }

      lcd.setCursor(0, 0);
      lcd.print("V:"); lcd.print(voltage, 1);
      lcd.print(" I:"); lcd.print(current, 1);
    }

    if (message.id == CAN_ID_SOC) {
      float soc = message.data[0];  // Try different byte if needed
      Serial.print("🔋 SOC: "); Serial.print(soc); Serial.println(" %");

      lcd.setCursor(0, 1);
      lcd.print("SOC:"); lcd.print(soc, 1); lcd.print(" %");

      logCsv = SD.open(csvName, FILE_WRITE);
      if (logCsv) {
        logCsv.print(now); logCsv.print(", , ,");
        logCsv.println(soc);
        logCsv.close();
      }
    }
  }

  delay(500);
}
