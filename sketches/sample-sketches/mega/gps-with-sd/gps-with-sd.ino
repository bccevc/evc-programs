/*
  6-8-10
  Aaron Weiss
  SparkFun Electronics
  
  Example GPS Parser based off of arduiniana.org TinyGPS examples.
  
  Parses NMEA sentences from an EM406 running at 4800bps into readable 
  values for latitude, longitude, elevation, date, time, course, and 
  speed. 
  
  For the SparkFun GPS Shield. Make sure the switch is set to DLINE.
  
  Once you get your longitude and latitude you can paste your 
  coordinates from the terminal window into Google Maps. Here is the 
  link for SparkFun's location.  
  http://maps.google.com/maps?q=40.06477,+-105.20997
  
  Uses the NewSoftSerial library for serial communication with your GPS, 
  so connect your GPS TX and RX pin to any digital pin on the Arduino, 
  just be sure to define which pins you are using on the Arduino to 
  communicate with the GPS module. 
  
  REVISIONS:
  1-17-11 
    changed values to RXPIN = 2 and TXPIN = to correspond with
    hardware v14+. Hardware v13 used RXPIN = 3 and TXPIN = 2.
  
*/

// In order for this sketch to work, you will need to download
// TinyGPS library from arduiniana.org and put them
// into the hardware->libraries folder in your ardiuno directory.
#include <Wire.h>
#include <SerLCD.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
//for 20x4 lcd display
//added for mega 40-41
#include <defaults.h>  //changed from defaultsMega.h

#include <global.h>
#include <SPI.h>
#include <SD.h>

SerLCD lcd;
//-SerLCD OpenLCD;
//lcd.begin(Wire);

// Define which pins you will use on the Arduino to communicate with your
// GPS. In this case, the GPS module's TX pin will connect to the
// Arduino's RXPIN which is pin 3.
#define RXPIN 4
#define TXPIN 5
//Set this value equal to the baud rate of your GPS
#define GPSBAUD 4800
float longitude,latitude,CurrentLatitude,CurrentLongitude,distance,TotalDist; //saved long/lat
float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude); // added 
//float SavedLongitude= -74.070808; //american dream
//float SavedLatitude= 40.809204; //american dream
float SavedLongitude;//= 0; 
float SavedLatitude;//= 0; 
float Constant = 121.32; //lines 53-61 from calc disc
const int chipSelect = 9;  //for SD card
// Create an instance of the TinyGPS object
TinyGPS gps;
// Initialize the NewSoftSerial library to the pins you defined above
SoftwareSerial uart_gps(RXPIN, TXPIN);
//-SoftwareSerial OpenLCD(6,7); // pin 7 = TX, pin 6 = RX (unused)

// This is where you declare prototypes for the functions that will be
// using the TinyGPS library.
void getgps(TinyGPS &gps);

// In the setup function, you need to initialize two serial ports; the
// standard hardware serial port (Serial()) to communicate with your
// terminal program an another serial port (NewSoftSerial()) for your
// GPS.
void setup() {
  // This is the serial rate for your terminal program. It must be this
  // fast because we need to print everything before a new sentence
  // comes in. If you slow it down, the messages might not be valid and
  // you will likely get checksum errors.

  {
    //code for SD
    Serial.begin(9600);
    Serial.print("Initializing SD card...");
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    pinMode(chipSelect, OUTPUT);

    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      // don't do anything more:
      return;
    }

    Serial.println("card initialized.");
  }
  //Sd ends here setup
  {
    Serial3.begin(GPSBAUD);
    Serial.begin(115200);
    Wire.begin();
    lcd.begin(Wire);

    Wire.setClock(400000);  //Optional - set I2C SCL to High Speed Mode of 400kHz

    lcd.clear();
    lcd.cursor();  //Turn on the underline cursor
                   //lcd.print("Watch the cursor!");

    //- OpenLCD.begin(9600); // set up serial port for 9600 baud  BobR
    delay(500);  // wait for display to boot up   BobR

    //Sets baud rate of your GPS
    uart_gps.begin(GPSBAUD);

    Serial.println("");
    Serial.println("GPS Shield QuickStart Example Sketch v12");
    Serial.println("       ...waiting for lock...           ");
    Serial.println("");
  }
}
// This is the main loop of the code. All it does is check for data on
// the RX pin of the ardiuno, makes sure the data is valid NMEA sentences,
// then jumps to the getgps() function.
void loop() {
  while (Serial3.available())  // While there is data on the RX pin...
  {
    int c = Serial3.read();  // load the data into a variable...
    if (gps.encode(c))        // if there is a new valid sentence...
    {
      Serial.println("       ...HERE IT COMES...           ");  //BobR
      getgps(gps); // then grab the data.
      CurrentLatitude = latitude;//added from lines 135-149
      CurrentLongitude = longitude; //added
      distance = calcDist(CurrentLatitude, CurrentLongitude, SavedLatitude, SavedLongitude); //added
      Serial.print("distance = " );
      Serial.print(distance,6);   
      TotalDist = distance + TotalDist; 
      Serial.print(" Trip: "); 
      Serial.print(TotalDist,2); 
      Serial.println();
      lcd.setCursor(0,2);
      lcd.print("Trip:");
      lcd.print(TotalDist,2);
      Serial.println();
      SavedLongitude = CurrentLongitude;
      SavedLatitude = CurrentLatitude;
    }
  }
}

// The getgps function will get and print the values we want.
void getgps(TinyGPS &gps) {
  // To get all of the data into varialbes that you can use in your code,
  // all you need to do is define variables and query the object for the
  // data. To see the complete list of functions see keywords.txt file in
  // the TinyGPS and NewSoftSerial libs.

  // Define the variables that will be used
  float latitude, longitude;
  // Then call this function
  gps.f_get_position(&latitude, &longitude);
  // You can now print variables latitude and longitude
  Serial.print("Lat/Long: ");
  Serial.print(latitude, 5);
  Serial.print(", ");
  Serial.println(longitude, 5);

  // make a string for assembling the data to log://made forSD
  String dataString = "";
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  // this opens the file and appends to the end of file
  // if the file does not exist, this will create a new file.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  /*
  Timestamp (ms), Lat, Long, Date (MM/DD/YYYY), Time (HH:MM:SS), Altitude (m), Course (deg), Speed (kmph)
  */

  if (dataFile) {
    int timeStamp = millis();
    //write to SD card
    dataFile.print(timeStamp);
    dataFile.print(",");
    //output also on Serial monitor for debugging
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

    // Here you can print the altitude and course values directly since
    // there is only one value for the function
    dataFile.print(gps.f_altitude());
    dataFile.print(",");
    // Same goes for course
    dataFile.print(gps.f_course());
    dataFile.print(",");
    // And same goes for speed
    dataFile.print(gps.f_speed_mph());

    dataFile.println();  // Create a new row
    dataFile.close();    //close file
    Serial.println();    //print to the serial port too:
  } else {
    Serial.println("error opening datalog.txt");
  }


  // Same goes for date and time
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
  //Since month, day, hour, minute, second, and hundr

  // BobR Write TIME to Display
  //- OpenLCD.write("                "); // clear display
  //-  OpenLCD.write("                ");

  //OpenLCD.write(254); // move cursor to beginning of first line
  //OpenLCD.write(192);
  //lcd.setCursor(0,0);

  //- OpenLCD.write("Time: "); OpenLCD.print(hour, DEC); OpenLCD.print(":");
  //- OpenLCD.print(minute, DEC); OpenLCD.print(":");
  //- OpenLCD.print(second, DEC); OpenLCD.print(":"); OpenLCD.print(hundredths, DEC);

  // lcd.init();
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

  //display speed
  lcd.setCursor(0, 1);
  lcd.print("Speed: ");
  lcd.print(gps.f_speed_mph());
  // Here you can print the altitude and course values directly since
  // there is only one value for the function
  Serial.print("Altitude (meters): ");
  Serial.println(gps.f_altitude());
  // Same goes for course
  Serial.print("Course (degrees): ");
  Serial.println(gps.f_course());
  // And same goes for speed
  Serial.print("Speed (mph): ");
  Serial.println(gps.f_speed_mph());
  Serial.println();

  // Here you can print statistics on the sentences (not used here).
  // unsigned long chars;
  // unsigned short sentences, failed_checksum;
  // gps.stats(&chars, &sentences, &failed_checksum);
  //Serial.print("Failed Checksums: ");Serial.print(failed_checksum);
  //Serial.println(); Serial.println();
}

float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude)
{
  
// HaverSine version
    const float Deg2Rad = 0.01745329252;               // (PI/180)  0.017453293, 0.0174532925
    //const double EarthRadius = 6372.795;              //6372.7976 In Kilo meters, will scale to other values
    const double EarthRadius = 3961;              // In Miles 3956
    //const float EarthRadius = 20908120.1;              // In feet  20908128.6
    float DeltaLatitude, DeltaLongitude, a, Distance;
if (SavedLongitude==0 ||SavedLatitude==0){
SavedLongitude = CurrentLongitude;
SavedLatitude = CurrentLatitude;  
}
    // degrees to radians
    CurrentLatitude = (CurrentLatitude + 180) * Deg2Rad;     // Remove negative offset (0-360), convert to RADS
    CurrentLongitude = (CurrentLongitude + 180) * Deg2Rad;
    SavedLatitude = (SavedLatitude + 180) * Deg2Rad;
    SavedLongitude = (SavedLongitude + 180) * Deg2Rad;

    DeltaLatitude = SavedLatitude - CurrentLatitude;
    DeltaLongitude = SavedLongitude - CurrentLongitude;

    a =(sin(DeltaLatitude/2) * sin(DeltaLatitude/2)) + cos(CurrentLatitude) * cos(SavedLatitude) * (sin(DeltaLongitude/2) * sin(DeltaLongitude/2));
    Distance = EarthRadius * (2 * atan2(sqrt(a),sqrt(1-a)));
  
    return(Distance);
  
}
