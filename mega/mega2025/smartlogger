// ******************************************************
// Written by Omer KILIC under Professor ROBERT ROLLER **
// ** Enhanced CAN Bus Data Logger with LCD Display    **
// ** Logs vehicle data to SD card (.txt + .csv files) **
// ******************************************************

#include <Canbus.h>
#include <mcp2515.h>
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
#define SD_CS 10

// -------- CAN Message IDs --------
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

// -------- Objects & Variables --------
SoftwareSerial uart_gps(RXPIN, TXPIN);
TinyGPS gps;
SerLCD lcd;
File logTxt;
File logCsv;

float latitude, longitude, lastLat = 0, lastLon = 0;
float totalDist = 0.0, energyUsed = 0.0;
unsigned long startTime;
String csvName = "";
String txtName = "";

// -------- Function Declarations --------
float readCANValue(int canID, int byteIndex = 0);
float readCAN16BitValue(int canID, int startIndex = 0, float scale = 10.0);
float calcDist(float lat1, float lon1, float lat2, float lon2);

// -------- Generate Filename from GPS Timestamp --------
String generateFileName(String prefix, String ext) {
  int year; byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  char filename[32];
  sprintf(filename, "%s_%02d%02d%04d_%02d%02d.%s", prefix.c_str(), month, day, year, hour, minute, ext.c_str());
  return String(filename);
}

// -------- Setup --------
void setup() {
  Serial.begin(115200);
  uart_gps.begin(GPSBAUD);
  Wire.begin();
  lcd.begin(Wire);
  lcd.clear();

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card init failed");
    return;
  }

  // Wait for valid GPS timestamp
  int year = 0; byte month, day, hour, minute, second, hundredths;
  while (year == 0) {
    while (uart_gps.available()) {
      gps.encode(uart_gps.read());
    }
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  }

  startTime = millis();
  csvName = generateFileName("log", "csv");
  txtName = generateFileName("log", "txt");

  logCsv = SD.open(csvName, FILE_WRITE);
  if (logCsv) {
    logCsv.println("GPS Date,GPS Time,Timestamp(ms),Discharge Enable,Charger Safe,GPS Speed (mph),Latitude,Longitude,MC Temp (°C),Pack Voltage (V),Pack Current (A),State of Charge (%),Lowest Cell V (V),Lowest Cell ID,Highest Cell V (V),Highest Cell ID,Balancing Active,Highest Therm Temp (°C),Thermistor ID,Error Flags,Time Increment (s),Distance Increment (mi),Energy Increment (kWh),Cumulative Distance (mi),Cumulative Energy (kWh),kWh per Mile,MPGe");
    logCsv.close();
  }
}

// -------- Loop --------
void loop() {
  unsigned long now = millis();
  float speed = gps.f_speed_mph();

  if (uart_gps.available()) {
    char c = uart_gps.read();
    gps.encode(c);
    gps.f_get_position(&latitude, &longitude);
  }

  // Read CAN data
  float current = readCAN16BitValue(CAN_ID_VOLTAGE_CURRENT, 0, 10.0);
  float voltage = readCANValue(CAN_ID_VOLTAGE_CURRENT, 3);
  float soc = readCANValue(CAN_ID_SOC);
  float lowCellV = readCAN16BitValue(CAN_ID_MIN_CELL_V, 0, 1000.0);
  float highCellV = readCAN16BitValue(CAN_ID_MAX_CELL_V, 0, 1000.0);
  float mcTemp = readCANValue(CAN_ID_MAX_TEMP);
  float discharge = readCANValue(CAN_ID_DISCHARGE);
  float chargerSafe = readCANValue(CAN_ID_CHARGER_SAFE);
  float balancing = readCANValue(CAN_ID_BALANCING);
  float errors = readCANValue(CAN_ID_ERROR_FLAGS);
  float lowCellID = readCANValue(CAN_ID_LOW_CELL_ID);
  float highCellID = readCANValue(CAN_ID_HIGH_CELL_ID);
  float thermID = readCANValue(CAN_ID_THERMISTOR_ID);
  float thermTemp = mcTemp;

  float distInc = calcDist(latitude, longitude, lastLat, lastLon);
  lastLat = latitude;
  lastLon = longitude;
  totalDist += distInc;

  float energyInc = (voltage * current / 1000.0) * (1.0 / 3600.0);
  energyUsed += energyInc;

  float kWhPerMile = (totalDist > 0) ? energyUsed / totalDist : 0;
  float mpge = (energyUsed > 0) ? (totalDist / energyUsed) * 33.7 : 0;

  int year; byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  String dateStr = String(month) + "/" + String(day) + "/" + String(year);
  String timeStr = String(hour) + ":" + String(minute) + ":" + String(second);

  // Log to CSV
  logCsv = SD.open(csvName, FILE_WRITE);
  if (logCsv) {
    logCsv.print(dateStr); logCsv.print(",");
    logCsv.print(timeStr); logCsv.print(",");
    logCsv.print(now); logCsv.print(",");
    logCsv.print(discharge); logCsv.print(",");
    logCsv.print(chargerSafe); logCsv.print(",");
    logCsv.print(speed, 2); logCsv.print(",");
    logCsv.print(latitude, 6); logCsv.print(",");
    logCsv.print(longitude, 6); logCsv.print(",");
    logCsv.print(mcTemp, 1); logCsv.print(",");
    logCsv.print(voltage, 1); logCsv.print(",");
    logCsv.print(current, 1); logCsv.print(",");
    logCsv.print(soc, 1); logCsv.print(",");
    logCsv.print(lowCellV, 3); logCsv.print(",");
    logCsv.print(lowCellID); logCsv.print(",");
    logCsv.print(highCellV, 3); logCsv.print(",");
    logCsv.print(highCellID); logCsv.print(",");
    logCsv.print(balancing); logCsv.print(",");
    logCsv.print(thermTemp, 1); logCsv.print(",");
    logCsv.print(thermID); logCsv.print(",");
    logCsv.print(errors); logCsv.print(",");
    logCsv.print(1); logCsv.print(",");
    logCsv.print(distInc, 4); logCsv.print(",");
    logCsv.print(energyInc, 6); logCsv.print(",");
    logCsv.print(totalDist, 2); logCsv.print(",");
    logCsv.print(energyUsed, 4); logCsv.print(",");
    logCsv.print(kWhPerMile, 6); logCsv.print(",");
    logCsv.println(mpge, 2);
    logCsv.close();
  }

  // Log to TXT
  logTxt = SD.open(txtName, FILE_WRITE);
  if (logTxt) {
    logTxt.print("[Time: "); logTxt.print(timeStr); logTxt.print("] ");
    logTxt.print("[Date: "); logTxt.print(dateStr); logTxt.print("] ");
    logTxt.print("Speed: "); logTxt.print(speed, 2); logTxt.print(" mph, ");
    logTxt.print("Lat: "); logTxt.print(latitude, 6); logTxt.print(", Lon: "); logTxt.print(longitude, 6);
    logTxt.print(", V: "); logTxt.print(voltage, 1); logTxt.print(" V");
    logTxt.print(", I: "); logTxt.print(current, 1); logTxt.print(" A");
    logTxt.print(", SoC: "); logTxt.print(soc, 1); logTxt.print("% ");
    logTxt.print(", MPGe: "); logTxt.print(mpge, 2);
    logTxt.println();
    logTxt.close();
  }

  delay(1000); // log every second
}

// -------- Helper Functions --------
float readCANValue(int canID, int byteIndex) {
  tCAN message;
  if (mcp2515_check_message() && mcp2515_get_message(&message)) {
    if (message.id == canID) return message.data[byteIndex];
  }
  return 0.0;
}

float readCAN16BitValue(int canID, int startIndex, float scale) {
  tCAN message;
  if (mcp2515_check_message() && mcp2515_get_message(&message)) {
    if (message.id == canID) {
      int raw = message.data[startIndex] * 256 + message.data[startIndex + 1];
      return raw / scale;
    }
  }
  return 0.0;
}

float calcDist(float lat1, float lon1, float lat2, float lon2) {
  if (lat2 == 0 && lon2 == 0) return 0.0;
  const float R = 3958.8;
  float dLat = radians(lat1 - lat2);
  float dLon = radians(lon1 - lon2);
  float a = sin(dLat / 2) * sin(dLat / 2) + cos(radians(lat2)) * cos(radians(lat1)) * sin(dLon / 2) * sin(dLon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}
