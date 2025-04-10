// ******************************************************
// Enhanced CAN Logger with SD + LCD + GPS Debug Support
// ******************************************************

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

// -------- Pin Definitions --------
#define RXPIN 4
#define TXPIN 5
#define GPSBAUD 4800
#define SD_CS 9
#define CAN_CS 10

// -------- CAN Message IDs --------
#define CAN_ID_VOLTAGE_CURRENT 0x03B

// -------- Global Objects --------
SoftwareSerial uart_gps(RXPIN, TXPIN);
TinyGPS gps;
SerLCD lcd;
File logTxt, logCsv;

float latitude, longitude, lastLat = 0, lastLon = 0;
float totalDist = 0.0, energyUsed = 0.0;
unsigned long startTime;
String csvName = "";
String txtName = "";

// -------- Setup --------
void setup() {
  delay(2000); // Add this line
  Serial.begin(115200);
  Serial.begin(115200);
  uart_gps.begin(GPSBAUD);
  Wire.begin();
  lcd.begin(Wire);
  lcd.clear();

  pinMode(SD_CS, OUTPUT);
  pinMode(CAN_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(CAN_CS, HIGH);

  // CAN Init
  digitalWrite(SD_CS, HIGH);     // Disable SD
  digitalWrite(CAN_CS, LOW);     // Enable CAN
  if (Canbus.init(CANSPEED_125)) {
    Serial.println("✅ CAN Init ok");
  } else {
    Serial.println("❌ CAN init failed");
  }
  digitalWrite(CAN_CS, HIGH);

  // SD Init
  digitalWrite(CAN_CS, HIGH);    // Disable CAN
  digitalWrite(SD_CS, LOW);      // Enable SD
  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD card init failed!");
    return;
  }
  Serial.println("✅ SD card initialized.");
  digitalWrite(SD_CS, HIGH);

  // Wait for GPS lock or timeout
  Serial.println("⏳ Waiting for GPS fix...");
  int year = 0; byte month, day, hour, minute, second, hundredths;
  unsigned long gpsWaitStart = millis();
  while (year == 0 && millis() - gpsWaitStart < 10000) {
    if (uart_gps.available()) {
      char c = uart_gps.read();
      Serial.write(c);  // Show raw GPS NMEA sentences
      gps.encode(c);
    }
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  }
  Serial.println("\n✅ GPS timestamp lock or timeout complete.");

  startTime = millis();
  csvName = "log.csv";
  txtName = "log.txt";

  // Create CSV Header
  digitalWrite(SD_CS, LOW);
  logCsv = SD.open(csvName, FILE_WRITE);
  if (logCsv) {
    logCsv.println("Timestamp(ms),Latitude,Longitude,Voltage(V),Current(A)");
    logCsv.close();
  }
  digitalWrite(SD_CS, HIGH);
}

// -------- Loop --------
void loop() {
  // Update GPS
  if (uart_gps.available()) {
    char c = uart_gps.read();
    gps.encode(c);
    gps.f_get_position(&latitude, &longitude);
  }

  // Check for CAN Message
  tCAN message;
  if (mcp2515_check_message() && mcp2515_get_message(&message)) {
    Serial.print("\n🔹 CAN ID: 0x"); Serial.print(message.id, HEX);
    Serial.print(" | Data: ");
    for (int i = 0; i < message.header.length; i++) {
      Serial.print(message.data[i], HEX); Serial.print(" ");
    }
    Serial.println();

    if (message.id == CAN_ID_VOLTAGE_CURRENT) {
      int rawCurrent = message.data[0] * 256 + message.data[1];
      int rawVoltage = message.data[3]; // usually 1 byte
      float current = rawCurrent / 10.0;
      float voltage = rawVoltage * 1.0;

      Serial.print("🔌 Current: "); Serial.print(current); Serial.println(" A");
      Serial.print("🔋 Voltage: "); Serial.print(voltage); Serial.println(" V");

      Serial.print("📍 Lat: "); Serial.print(latitude, 6);
      Serial.print(" | Lon: "); Serial.println(longitude, 6);

      // Save to SD card
      digitalWrite(SD_CS, LOW);
      logCsv = SD.open(csvName, FILE_WRITE);
      if (logCsv) {
        logCsv.print(millis()); logCsv.print(",");
        logCsv.print(latitude, 6); logCsv.print(",");
        logCsv.print(longitude, 6); logCsv.print(",");
        logCsv.print(voltage, 2); logCsv.print(",");
        logCsv.println(current, 2);
        logCsv.close();
      }
      digitalWrite(SD_CS, HIGH);
    }
  }

  delay(1000);
}
