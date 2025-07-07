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
float longitude, latitude, course, mph, altitude;
int year;
byte month, day, hour, minute, second, hundredths;
const int CHIPSELECT = 9;
TinyGPS gps;
SerLCD lcd;
float packCurrent = 0.0, packVoltage = 0.0, batteryTemp = 0.0;
unsigned int lowestCellVoltage = 0;
int lowestCellID;

// Function prototypes
SoftwareSerial uart_gps(RXPIN, TXPIN);
void getgps(TinyGPS &gps);

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
    return 1;
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
}

void loop() {
  timeStamp = millis();
  // Get GPS data as global variables
  while (Serial3.available()) { // While there is data on Serial3, which is where the GPS is hooked up to; using IF doesn't wait for a valid sentence
    int c = Serial3.read(); // Store data from Serial3
    if (gps.encode(c)) { // Decode GPS message
      getgps(gps);
    }
  }
  // Write GPS data and timestamp to Serial Monitor
  // Serial.print(timeStamp);
  // Serial.print(" | ");
  // Serial.print("Course: ");
  // Serial.print(course);
  // Serial.print(" | ");
  // Serial.print("Latitude: ");
  // Serial.print(latitude);
  // Serial.print(" | ");
  // Serial.print("Longitude: ");
  // Serial.print(longitude);
  // Serial.print(" | ");
  // Serial.print("Altitude: ");
  // Serial.print(altitude);
  // Serial.print(" | ");
  // Serial.print("MPH: ");
  // Serial.print(mph);
  // Serial.print(" | ");
  // Serial.print(month); // Print date
  // Serial.print("/");
  // Serial.print(day);
  // Serial.print("/");
  // Serial.print(year);
  // Serial.print(" | ");
  // int edtTime = hour - 4; // Convert timezone to EDT
  // Serial.print(edtTime); // Print time
  // Serial.print(":");
  // Serial.print(minute);
  // Serial.print(":");
  // Serial.print(second);
  // Serial.print(".");
  // Serial.println(hundredths);
  
  // Get CAN bus messages as global
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
        packVoltage = message.data[3];
      }
      // Get battery temp, lowest cell ID, lowest cell voltage
      if (message.id == 0x123) {
        lowestCellVoltage = 0;
        // Get lowest cell voltage
        lowestCellVoltage = ((unsigned int)message.data[0] << 8) | message.data[1];
        
        // for (int i = 0; i < 2; i++) {
        //   unsigned int tmpByte = message.data[i];
        //   lowestCellVoltage = lowestCellVoltage + (tmpByte*(pow(256, 1-i)));
        // }

        lowestCellID = message.data[2];
        batteryTemp = message.data[3];
      }
    }
  }

  // Write CAN bus messages to console with timestamp
  Serial.print("Pack current: ");
  Serial.print(packCurrent, 1);
  Serial.print(" | ");
  Serial.print("Pack voltage: ");
  Serial.print(packVoltage);
  Serial.print(" | ");
  Serial.print("Battery temp: ");
  Serial.print(batteryTemp);
  Serial.print(" | ");
  Serial.print("Lowest cell ID: ");
  Serial.print(lowestCellID);
  Serial.print(" | ");
  Serial.print("Lowest cell voltage: ");
  Serial.println(lowestCellVoltage / 10000.0, 3);

  // Compute distance, trip, power, etc.
  // Write all data to SD, LCD, console
}

void getgps(TinyGPS &gps) {
  gps.f_get_position(&latitude, &longitude);
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  course = gps.f_course();
  mph = gps.f_speed_mph();
  altitude = gps.f_altitude();
}