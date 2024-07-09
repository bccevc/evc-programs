/*
Copied from SparkFun_SerialLCD_Demo
*/

#include <Canbus.h>
#include <defaults.h>  // Changed from defaultsMega.h
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <SerLCD.h>
#include <SoftwareSerial.h>

#define RXPIN 4
#define TXPIN 5
#define GPSBAUD 4800 // Baud rate of GPS

// Function prototypes
float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude);
void getgps(TinyGPS &gps);

const int chipSelect = 9; // For SD card

float longitude, latitude, CurrentLatitude, CurrentLongitude, distance, TotalDist;
float SavedLongitude;   
float SavedLatitude;

TinyGPS gps;
SerLCD lcd;

// Initialize the NewSoftSerial library to the pins you defined above
SoftwareSerial uart_gps(RXPIN, TXPIN);

void setup() {
  delay(500);
  Serial.begin(9600);
  Serial.println("CAN Read - Testing receival of CAN Bus message");
  delay(1000);

  // Initialize MCP2515 CAN controller at the specified speed
  if (Canbus.init(CANSPEED_125)) {
    Serial.println("CAN Init ok");
  }
  else {
    Serial.println("Can't init CAN");
  }  

  delay(1000);
  Wire.begin();
  lcd.begin(Wire);
  
  Serial.print("Initializing SD card...");
  // Make sure that the default chip select pin is set to output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);

  // See if the card is present and can be initialized
  if (!SD.begin(chipSelect)) {
    Serial.print("Card failed, or not present");
    return;
  }

  Serial.print("card initialized.");
  Serial3.begin(GPSBAUD);
  uart_gps.begin(GPSBAUD);

  Wire.setClock(400000); // Optional - set I2C SCL to High Speed Mode of 400kHz

  lcd.clear();
  lcd.cursor(); //Turn on the underline cursor
  delay(500);

  Serial.println("");
  Serial.println("GPS Shield QuickStart Example Sketch v12");
  Serial.println("       ...waiting for lock...           ");
  Serial.println("");
}

void loop() {
  while (Serial3.available()) {  // While there is data on the RX pin
    int c = Serial3.read();  // Load the data into a variable
    if (gps.encode(c)) {        // If there is a new valid sentence
      Serial.println("       ...HERE IT COMES...           ");
      getgps(gps); // then grab the data
      CurrentLatitude = latitude;
      CurrentLongitude = longitude;
      distance = calcDist(CurrentLatitude, CurrentLongitude, SavedLatitude, SavedLongitude);
      Serial.print("distance = " );
      Serial.print(distance, 6);   
      TotalDist = distance + TotalDist; 
      Serial.print(" Trip: "); 
      Serial.print(TotalDist, 2); 
      Serial.println();
      lcd.setCursor(11, 3);
      lcd.print("Trip:");
      lcd.print(TotalDist, 2);
      Serial.println();
      SavedLongitude = CurrentLongitude;
      SavedLatitude = CurrentLatitude;

      tCAN message;
      if (mcp2515_check_message()) {
        if (mcp2515_get_message(&message)) {      
          Serial.print("ID: ");
          Serial.print(message.id,HEX);
          Serial.print(", ");
          Serial.print("Data: ");
          Serial.print(message.header.length,DEC);
          Serial.print("  ");
          for (int i = 0; i < message.header.length; i++) {	
            Serial.print(message.data[i],HEX);
            Serial.print(" ");
          }
          Serial.println("");
          Serial.print(message.id,HEX);
          Serial.print(" Id "); 
              
          if (message.id == 0x03B) {
            lcd.setCursor(0,2);
            lcd.write("current:");
            // Capture current as decimal value and display on LCD
            int IDEC = 0;
            for (int i = 0; i < 2; i++) { // Removed 2 and changed to 1 so it reads one byte
              Serial.print(message.data[i],HEX);
              Serial.print(" Hex "); 
              int MDATA = (message.data[i]); // LeahAna when we put a ",DEC" gives us constant DEC value of 10
              Serial.print((float)MDATA/10,1); // LeahAna added this to add the decimal place
              Serial.print(" Dec "); 
              IDEC=IDEC+(MDATA*(pow(256, 1-i))); // BOB changed "message.data[i]" to MDATA
              Serial.print(IDEC);
              Serial.print(" IDEC ");
            }
            
            lcd.print((float)IDEC/10, 1); // LeahAna crossed out line below; aded the float and put IDEC instead of MDATA
            Serial.println("Final");
            lcd.setCursor(0,3);  
            lcd.write("voltage:");
            for (int i = 3; i < 4; i++) { // AnaLeah changed i=2 to i=3.. gets rid of leading zero
              lcd.print(message.data[i],DEC); //LeahAna changed HEX to DEC
            }   
          }      
          String dataString = "";    
          File dataFile = SD.open("datalog.txt", FILE_WRITE);
          if (dataFile) {  
            int timeStamp = millis();
            int IDEC = 0;
            // Write to uSD card: timestamp, current, voltage
            dataFile.print(timeStamp);
            dataFile.print(",");
            dataFile.print((float)IDEC/10,1);
            dataFile.print(",");
            int i = 3;
            dataFile.print(message.data[i],DEC);                        
          }
          dataFile.println(); // New row
          dataFile.close();
          delay(100);
          Serial.println();
        }
        else {
          Serial.println("error opening datalog.txt");
        } 
      }
    }
  }
}

void getgps(TinyGPS &gps) {
 /*
 To get all of the data into varialbes that you can use in your code,
 all you need to do is define variables and query the object for the
 data. To see the complete list of functions see keywords.txt file in
 the TinyGPS and NewSoftSerial libs. 
 */

  float latitude, longitude;

  gps.f_get_position(&latitude, &longitude);

  Serial.print("Lat/Long: ");
  Serial.print(latitude, 5);
  Serial.print(", ");
  Serial.println(longitude, 5);

  String dataString = "";
  // Only one file can be open at a time. Close before opening another. If file has same name, content is appended to the end.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // Timestamp (ms), Lat, Long, Date (MM/DD/YYYY), Time (HH:MM:SS), Altitude (m), Course (deg), Speed (kmph)
  if (dataFile) {
    int timeStamp = millis();

    dataFile.print(timeStamp);
    dataFile.print(",");

    Serial.print("Timestamp: ");
    Serial.print(timeStamp);
    Serial.print(",");

    dataFile.print(latitude, 5);
    dataFile.print(",");
    dataFile.print(longitude, 5);
    dataFile.print(",");

    int year;
    byte month, day, hour, minute, second, hundredths;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);

    // Date
    dataFile.print(month, DEC);
    dataFile.print("/");
    dataFile.print(day, DEC);
    dataFile.print("/");
    dataFile.print(year);

    // Time
    int echour = hour - 4;
    dataFile.print(",");
    dataFile.print(echour, DEC);
    dataFile.print(":");
    dataFile.print(minute, DEC);
    dataFile.print(":");
    dataFile.print(second, DEC);
    dataFile.print(".");
    dataFile.print(hundredths, DEC);
    dataFile.print(",");

    // Here you can print the altitude and course values directly since there is only one value for the function
    dataFile.print(gps.f_altitude());
    dataFile.print(",");
    dataFile.print(gps.f_course());
    dataFile.print(",");
    dataFile.print(gps.f_speed_kmph());

    dataFile.println();
    dataFile.close();
    Serial.println();
  }
  else {
    Serial.println("error opening datalog.txt");
  }

  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths);

  // Print data and time
  Serial.print("Date: ");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(day, DEC);
  Serial.print("/");
  Serial.print(year);
  int echour = hour - 4;
  Serial.print("  Time: ");
  Serial.print(echour, DEC);
  Serial.print(":");
  Serial.print(minute, DEC);
  Serial.print(":");
  Serial.print(second, DEC);
  Serial.print(".");
  Serial.println(hundredths, DEC);

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(echour, DEC);
  lcd.print(":");
  lcd.print(minute, DEC);
  lcd.print(":");
  lcd.print(second, DEC);
  lcd.print(":");
  lcd.print(hundredths, DEC);
  lcd.print("  ");

  lcd.setCursor(0, 1);
  lcd.print("Speed: ");
  lcd.print(gps.f_speed_kmph());
  Serial.print("Altitude (meters): ");
  Serial.println(gps.f_altitude());
  Serial.print("Course (degrees): ");
  Serial.println(gps.f_course());
  Serial.print("Speed (kmph): ");
  Serial.println(gps.f_speed_kmph());
  Serial.println();
}

float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude) {
  // HaverSine version
  const float Deg2Rad = 0.01745329252;  // (PI/180)  0.017453293, 0.0174532925
  const double EarthRadius = 3961;  // Earth radius in miles
  float DeltaLatitude, DeltaLongitude, a, Distance;

  if (SavedLongitude == 0 || SavedLatitude == 0) {
    SavedLongitude = CurrentLongitude;
    SavedLatitude = CurrentLatitude;
  }

  // Degrees to radians
  CurrentLatitude = (CurrentLatitude + 180) * Deg2Rad;  // Remove negative offset (0-360), convert to RADS
  CurrentLongitude = (CurrentLongitude + 180) * Deg2Rad;
  SavedLatitude = (SavedLatitude + 180) * Deg2Rad;
  SavedLongitude = (SavedLongitude + 180) * Deg2Rad;

  DeltaLatitude = SavedLatitude - CurrentLatitude;
  DeltaLongitude = SavedLongitude - CurrentLongitude;

  a = (sin(DeltaLatitude / 2) * sin(DeltaLatitude / 2)) + cos(CurrentLatitude) * cos(SavedLatitude) * (sin(DeltaLongitude / 2) * sin(DeltaLongitude / 2));
  Distance = EarthRadius * (2 * atan2(sqrt(a), sqrt(1 - a)));

  return (Distance);
}