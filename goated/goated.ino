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
float longitude, latitude, course, mph;
const int CHIPSELECT = 9;
TinyGPS gps;

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
    Serial.println("WARNING: Can't init CAN.")
  }
  delay(1000);
  // Test SD card
  Serial.println("INITIALIZE: SD Card");
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
  while (Serial3.available()) { // While there is data on Serial3, which is where the GPS is hooked up to
    int c = Serial3.read(); // Store data from Serial3
    if (gps.encode(c)) { // Decode GPS message
      getgps(gps);
    }
  }
  // Write GPS data and timestamp to console
  // Get CAN bus messages as global
  // Write CAN bus messages to console with timestamp
  // Compute distance, trip, power, etc.
  // Write all data to SD, LCD, console
}

void getgps(TinyGPS &gps) {
  gps.f_get_position(&latitude, &longitude);
  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);
  course = gps.f_course();
  mph = gps.f_speed_mph();
}