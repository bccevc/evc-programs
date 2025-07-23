#include <Canbus.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <SerLCD.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>

#define RXPIN 4
#define TXPIN 5
#define GPSBAUD 4800

// Global variables
unsigned long timeStamp;
float longitude, latitude, course, mph, altitude, packCurrent = 0.0, batteryTemp = 0.0, distance, savedLatitude, savedLongitude, trip, deltaTime, savedTimestamp, mpge = 0.0, power, speed, packVoltage = 0.0, lowestCellVoltageOutput = 0.0; // lowestCellVoltageOutut needs to be implicitly declared as a float, is initially processed as an unsigned int
int year, lowestCellID, packSOC;
byte month, day, hour, minute, second, hundredths;
unsigned int lowestCellVoltage = 0;
double energyUsed = 0.0, cumulativeEnergy = 0.0;
TinyGPS gps;
SerLCD lcd;

// Constant variables
const int CHIPSELECT = 9;
const float MPGE_CONSTANT = 121.32;

// Function prototypes
SoftwareSerial uart_gps(RXPIN, TXPIN);
void getGPS(TinyGPS &gps);
float calcDist(float latitude, float longitude, float &savedLatitude, float &savedLongitude);
void printToSerialMonitor();
void printToSD();
void printToLCD();

void setup() {
  Serial.begin(115200);
  // Test CAN connection
  Serial.println("TESTING: CAN Init");
  delay(1000);
  if (Canbus.init(CANSPEED_125)) {
    Serial.println("SUCCESS: CAN OK!");
  }
  else {
    Serial.println("WARNING: Can't init CAN.");
  }
  delay(1000);
  // Test SD card
  Serial.println("INITIALIZING: SD Card");
  pinMode(CHIPSELECT, OUTPUT); // Set default chip select pin
  if (!SD.begin(CHIPSELECT)) {
    Serial.print("ERROR: Initializing SD card failed. Check if present.");
    while(1); // Hang
  }
  // Initialize LCD
  Wire.begin();
  lcd.begin(Wire);
  Wire.setClock(400000); // Set I2C SCL to High Speed Mode of 400kHz (optional)
  lcd.clear();
  lcd.cursor();
  delay(500); // Wait for display to boot
  // Initialize GPS
  Serial3.begin(GPSBAUD);
  uart_gps.begin(GPSBAUD); // Set GPS baud rate
  Serial.println("WAITING: Wait for GPS lock.");

  savedTimestamp = millis(); // For calculating deltaTime
}

void loop() {
  timeStamp = millis();
  // Get GPS data as global variables
  while (Serial3.available()) { // While there is data on Serial3, which is where the GPS is hooked up to; using IF doesn't wait for a valid sentence
    int c = Serial3.read(); // Store data from Serial3
    if (gps.encode(c)) { // Decode GPS message
      getGPS(gps);

      // Compute distance
      distance = calcDist(latitude, longitude, savedLatitude, savedLongitude);
      Serial.print("Distance: ");
      Serial.println(distance, 6);
      
      // Compute trip
      trip = trip + distance;

      // Get CAN bus messages
      tCAN message;
      if (mcp2515_check_message()) {
        if (mcp2515_get_message(&message)) {
          // Wait to get data to prevent using stale data
          // Get pack current, pack voltage
          if (message.id == 0x03B) {
            // Get pack current - unsure of the logic here
            for (int i = 0; i < 2; i++) {
              int tmpByte = message.data[i];
              packCurrent = tmpByte * (pow(256, 1 - i));
              packCurrent /= 10;
            }
            // Get pack voltage - is being sent as 2 bytes
            packVoltage = ((unsigned int)message.data[2] << 8) | message.data[3];
            packVoltage /= 10; // Must be done separately to get the decimal value
          }
          // Get battery temp, lowest cell ID, lowest cell voltage
          if (message.id == 0x123) {
            lowestCellVoltage = 0;
            for (int i = 0; i < 2; i++) {
              unsigned int tmpByte = message.data[i];
              lowestCellVoltage = lowestCellVoltage + (tmpByte * (pow(256, 1 - i)));
            }
            lowestCellID = message.data[2];
            batteryTemp = message.data[3];
          }
          // Get pack SOC
          if (message.id == 0x6B0) {
            packSOC = message.data[1];
          }
        }
      }
      // Intermediate conversions for pack voltage and lowest cell voltage
      packVoltage /= 1.0;
      lowestCellVoltageOutput = lowestCellVoltage / 10000.0;

      // Compute power
      power = packVoltage * packCurrent;

      // Compute energy used over an elapsed time (deltaTime) to compute cumulative energy
      deltaTime = timeStamp - savedTimestamp;
      savedTimestamp = timeStamp;
      // Convert deltaTime to hours from ms
      deltaTime /= 1000; // ms to seconds
      deltaTime /= 60; // seconds to minutes
      deltaTime /= 60; // minutes to hours
      energyUsed = (power * deltaTime) / 1000; // In kwh

      // Compute cumulative energy
      cumulativeEnergy = energyUsed + cumulativeEnergy;

      // Compute MPGe
      if (cumulativeEnergy > 0) {
        mpge = (trip / cumulativeEnergy) * MPGE_CONSTANT;
      }
      
      // Write all data to console, SD, LCD
      printToSerialMonitor();
      printToSD();
      printToLCD();
    }
  }
}

void getGPS(TinyGPS &gps) {
  gps.f_get_position(&latitude, &longitude);
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  course = gps.f_course();
  mph = gps.f_speed_mph();
  altitude = gps.f_altitude();
}

// calcDist() uses the Haversine formula
float calcDist(float latitude, float longitude, float &savedLatitude, float &savedLongitude) {
  // const float DEG_TO_RAD = 0.01745329252;               
  const float EARTH_RADIUS = 3963.1906;          
  float deltaLatitude, deltaLongitude, a, distance;
  if (savedLongitude == 0 || savedLatitude == 0) {
    savedLongitude = longitude;
    savedLatitude = latitude;
  }
  Serial.println("-----------------------");

  Serial.print("Saved Latitude: ");
  Serial.println(savedLatitude, 6);
  Serial.print("Saved Longitude: ");
  Serial.println(savedLongitude, 6);

  Serial.print("Latitude: ");
  Serial.println(latitude, 6);
  Serial.print("Longitude: ");
  Serial.println(longitude, 6);

  Serial.println("-----------------------");

  // Convert degrees to radians
  float tmpLat = (latitude + 180) * DEG_TO_RAD;   
  // Remove negative offset (0-360), convert to RADS
  float tmpLong = (longitude + 180) * DEG_TO_RAD;
  savedLatitude = (savedLatitude + 180) * DEG_TO_RAD;
  savedLongitude = (savedLongitude + 180) * DEG_TO_RAD;
  deltaLatitude = savedLatitude - tmpLat;
  Serial.print("deltalatitude:: ");
  Serial.println(deltaLatitude, 6);

  deltaLongitude = savedLongitude - tmpLong;
  Serial.print("deltalongitude:: ");
  Serial.println(deltaLongitude, 6);
  a = (sin(deltaLatitude / 2) * sin(deltaLatitude / 2)) + cos(tmpLat) * cos(savedLatitude) * (sin(deltaLongitude / 2) * sin(deltaLongitude / 2));
  distance = EARTH_RADIUS * (2 * atan2(sqrt(a), sqrt(1 - a)));
  
  return distance;
}

void printToSerialMonitor() {
  // Write GPS data and timestamp
  Serial.print(timeStamp);
  Serial.print(" | ");
  Serial.print("Course: ");
  Serial.print(course);
  Serial.print(" | ");
  Serial.print("Latitude: ");
  Serial.print(latitude, 4);
  Serial.print(" | ");
  Serial.print("Longitude: ");
  Serial.print(longitude, 4);
  Serial.print(" | ");
  Serial.print("Altitude: ");
  Serial.print(altitude);
  Serial.print(" | ");
  Serial.print("MPH: ");
  Serial.print(mph);
  Serial.print(" | ");
  Serial.print(month); // Print date
  Serial.print("/");
  Serial.print(day);
  Serial.print("/");
  Serial.print(year);
  Serial.print(" | ");
  int edtTime = hour - 4; // Convert timezone to EDT
  Serial.print(edtTime); // Print time
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.print(second);
  Serial.print(".");
  Serial.println(hundredths);
  // CAN bus messages
  // Serial.print("Pack current: ");
  // Serial.print(packCurrent, 1);
  // Serial.print(" | ");
  // Serial.print("Pack voltage: ");
  // Serial.print(packVoltage, 1);
  // Serial.print(" | ");
  // Serial.print("Battery temp: ");
  // Serial.print(batteryTemp);
  // Serial.print(" | ");
  // Serial.print("Lowest cell ID: ");
  // Serial.print(lowestCellID);
  // Serial.print(" | ");
  // Serial.print("Lowest cell voltage: ");
  // Serial.print(lowestCellVoltageOutput, 3);
  // Serial.print(" | ");
  // Serial.print("Pack SOC: ");
  // Serial.println(packSOC);
  // // Print computed values
  Serial.print("Distance: ");
  Serial.println(distance, 6);
  Serial.print("Trip: ");
  Serial.println(trip, 2);
  // Serial.print("Power: ");
  // Serial.println(power);
  // Serial.print("Energy used: ");
  // Serial.println(energyUsed, 6);
  // Serial.print("Cumulative energy: ");
  // Serial.println(cumulativeEnergy, 4);
  // Serial.print("MPGe: ");
  // Serial.println(mpge, 9);
  // Serial.println();
}

void printToSD() {
  File outFile = SD.open("output.txt", FILE_WRITE);
  // Print lat, long, date, time, course, speed, distance, trip, pack current, pack voltage, lowest cell ID, lowest cell voltage, battery temperature, energy used, cumulative energy, mpge
  if (outFile) {
    outFile.print(timeStamp);
    outFile.print(",");
    outFile.print(latitude, 5);
    outFile.print(",");
    outFile.print(longitude, 5);
    outFile.print(",");
    outFile.print(month); // Print date
    outFile.print("/");
    outFile.print(day);
    outFile.print("/");
    outFile.print(year);
    outFile.print(",");
    int edtTime = hour - 4; // Convert timezone to EDT
    outFile.print(edtTime); // Print time
    outFile.print(":");
    outFile.print(minute);
    outFile.print(":");
    outFile.print(second);
    outFile.print(".");
    outFile.print(hundredths);
    outFile.print(",");
    outFile.print(course);
    outFile.print(",");
    outFile.print(speed);
    outFile.print(",");
    outFile.print(distance);
    outFile.print(",");
    outFile.print(trip);
    outFile.print(",");
    outFile.print(packCurrent, 1);
    outFile.print(",");
    outFile.print(packVoltage, 1);
    outFile.print(",");
    outFile.print(lowestCellID);
    outFile.print(",");
    outFile.print(lowestCellVoltageOutput, 3);
    outFile.print(",");
    outFile.print(batteryTemp);
    outFile.print(",");
    outFile.print(energyUsed, 6);
    outFile.print(",");
    outFile.print(cumulativeEnergy, 4);
    outFile.print(",");
    outFile.print(mpge, 9);
    outFile.print(",");
    outFile.print(packSOC);
    outFile.println();
  }
  outFile.close();
}

void printToLCD() {
  lcd.setCursor(10,3);
  lcd.print("TRIP:");
  lcd.print(trip, 2);

  lcd.setCursor(0,2); 
  lcd.print(packCurrent, 1);
  lcd.print("A");

  lcd.setCursor(7,2);
  lcd.print(packVoltage, 1);
  lcd.print("V");

  lcd.setCursor(0,1);
  lcd.print("BT:");
  lcd.print(batteryTemp);
  lcd.print("C");

  lcd.setCursor(17,1);
  lcd.print("#");
  lcd.print(lowestCellID);

  lcd.setCursor(13,2);
  lcd.print(lowestCellVoltageOutput, 3);
  lcd.print("v");

  lcd.setCursor(0, 0);
  lcd.print(speed);
  lcd.print("MPH ");

  lcd.setCursor(8, 1);
  lcd.print(" ");

  lcd.setCursor(0, 3);
  lcd.print(cumulativeEnergy, 3);
  lcd.print("KWH");

  lcd.setCursor(9, 0);
  lcd.print("RPM:____");
  lcd.setCursor(8, 1);
  lcd.print("MC:___C"); 
}