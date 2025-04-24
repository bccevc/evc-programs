// === Refined Arduino CAN Logger Using SparkFun CANbus.h ===
// ✅ Now uses legacy working structure with Canbus.h + mcp2515_get_message()
// ✅ Supports IDs: 0x03B, 0x3CB, 0x6B0, 0x123
// ✅ Logs to SD, rotates LCD, and includes GPS distance + MPGe

#include <Canbus.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include <SerLCD.h>
#include <math.h>

#define uart_gps Serial3
#define GPSBAUD 4800
#define SD_CS 53

// CAN IDs
#define ID_VOLT_CURRENT 0x03B
#define ID_SOC_TEMP     0x3CB
#define ID_MISC_FLAGS   0x6B0
#define ID_LOW_CELL     0x123

TinyGPS gps;
SerLCD lcd;
File logCsv;

float voltage = 0, current = 0, soc = 0, lowCellV = 0;
int cellID = -1, highTemp = -1;
unsigned long ampHours = 0;
float latitude, longitude, lastLat = 0, lastLon = 0;
float totalDist = 0.0, energyUsed = 0.0, mpge = 0.0;
unsigned long lastRotate = 0;
int screenIndex = 0;

String csvName;
unsigned long startTime;

String generateFileName(String prefix, String ext) {
  int year; byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  char filename[32];
  sprintf(filename, "%s_%02d%02d%04d_%02d%02d.%s", prefix.c_str(), month, day, year, hour, minute, ext.c_str());
  return String(filename);
}

float calcDistance(float lat1, float lon1, float lat2, float lon2) {
  const float R = 3961.0;
  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);
  lat1 = radians(lat1); lat2 = radians(lat2);
  float a = sin(dLat/2)*sin(dLat/2) + cos(lat1)*cos(lat2)*sin(dLon/2)*sin(dLon/2);
  return R * 2 * atan2(sqrt(a), sqrt(1-a));
}

void rotateLCD() {
  lcd.clear();
  switch (screenIndex) {
    case 0:
      lcd.setCursor(0,0); lcd.print("V:"); lcd.print(voltage,1); lcd.print("V");
      lcd.setCursor(0,1); lcd.print("I:"); lcd.print(current,1); lcd.print("A");
      break;
    case 1:
      lcd.setCursor(0,0); lcd.print("SOC:"); lcd.print(soc,1); lcd.print("%");
      lcd.setCursor(0,1); lcd.print("Temp:"); lcd.print(highTemp); lcd.print("C");
      break;
    case 2:
      lcd.setCursor(0,0); lcd.print("LowCell:"); lcd.print(lowCellV,3); lcd.print("V");
      lcd.setCursor(0,1); lcd.print("Cell ID:"); lcd.print(cellID);
      break;
    case 3:
      lcd.setCursor(0,0); lcd.print("Ah:"); lcd.print(ampHours);
      lcd.setCursor(0,1); lcd.print("Trip:"); lcd.print(totalDist,2);
      break;
  }
  screenIndex = (screenIndex + 1) % 4;
}

void setup() {
  delay(2000);
  Serial.begin(115200);
  uart_gps.begin(GPSBAUD);
  Wire.begin();
  lcd.begin(Wire); lcd.setAddress(0x72); lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Booting...");

  if (!SD.begin(SD_CS)) {
    lcd.setCursor(0,1); lcd.print("SD FAIL"); while (1);
  }
  lcd.setCursor(0,1); lcd.print("SD OK");

  if (Canbus.init(CANSPEED_125)) {
    lcd.setCursor(0,2); lcd.print("CAN OK");
  } else {
    lcd.setCursor(0,2); lcd.print("CAN FAIL"); while (1);
  }

  int year = 0; byte month, day, hour, minute, second, hundredths;
  while (year == 0) {
    while (uart_gps.available()) gps.encode(uart_gps.read());
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  }
  lcd.setCursor(0,3); lcd.print("GPS OK");

  startTime = millis();
  csvName = generateFileName("evc_log", "csv");
  logCsv = SD.open(csvName, FILE_WRITE);
  if (logCsv) {
    logCsv.println("Time,Voltage,Current,SOC,LowCellV,CellID,Temp,AmpHours,Trip,Energy,MPGe");
    logCsv.close();
  }
  delay(3000); lcd.clear();
}

void loop() {
  unsigned long now = millis();

  if (uart_gps.available()) {
    gps.encode(uart_gps.read());
    gps.f_get_position(&latitude, &longitude);
    if (lastLat != 0 && lastLon != 0) {
      float delta = calcDistance(lastLat, lastLon, latitude, longitude);
      totalDist += delta;
      energyUsed = (voltage * current * (now - startTime) / 3600000.0) / 1000.0;
      if (energyUsed > 0) mpge = (totalDist / energyUsed) * 33.7;
    }
    lastLat = latitude;
    lastLon = longitude;
  }

  tCAN message;
  if (mcp2515_check_message() && mcp2515_get_message(&message)) {
    Serial.print("ID: 0x"); Serial.print(message.id, HEX);
    Serial.print(" | Data: ");
    for (int i = 0; i < message.header.length; i++) {
      if (message.data[i] < 0x10) Serial.print("0");
      Serial.print(message.data[i], HEX); Serial.print(" ");
    }
    Serial.println();

    if (message.id == ID_VOLT_CURRENT) {
      int16_t rawCurrent = (message.data[0] << 8) | message.data[1];
      current = rawCurrent / 10.0;
      voltage = message.data[2];
    } else if (message.id == ID_SOC_TEMP) {
      soc = message.data[2];
      highTemp = message.data[3];
    } else if (message.id == ID_MISC_FLAGS) {
      ampHours = ((uint32_t)message.data[3] << 24) | ((uint32_t)message.data[4] << 16) | ((uint32_t)message.data[5] << 8) | (message.data[6]);
    } else if (message.id == ID_LOW_CELL) {
      int rawLowV = (message.data[0] << 8) | message.data[1];
      lowCellV = rawLowV / 10000.0;
      cellID = message.data[2];
    }
  }

  static unsigned long lastLog = 0;
  if (now - lastLog > 1000) {
    logCsv = SD.open(csvName, FILE_WRITE);
    if (logCsv) {
      logCsv.print(now); logCsv.print(",");
      logCsv.print(voltage); logCsv.print(",");
      logCsv.print(current); logCsv.print(",");
      logCsv.print(soc); logCsv.print(",");
      logCsv.print(lowCellV); logCsv.print(",");
      logCsv.print(cellID); logCsv.print(",");
      logCsv.print(highTemp); logCsv.print(",");
      logCsv.print(ampHours); logCsv.print(",");
      logCsv.print(totalDist); logCsv.print(",");
      logCsv.print(energyUsed); logCsv.print(",");
      logCsv.println(mpge);
      logCsv.close();
    }
    lastLog = now;
  }

  if (now - lastRotate > 3000) {
    rotateLCD();
    lastRotate = now;
  }
}
