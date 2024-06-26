#include <Canbus.h>
#include <defaults.h>  //changed from defaultsMega.h
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>
#include <TinyGPS.h>

#include <SPI.h>
#include <SD.h>
const int chipSelect = 9;

#include <Wire.h>
#include <SerLCD.h>
/////////////////copied from SparkFun_SerialLCD_Demo/////////

#include <SoftwareSerial.h>

SerLCD lcd;
#define RXPIN 4
#define TXPIN 5
//Set this value equal to the baud rate of your GPS
#define GPSBAUD 4800
float longitude, latitude, CurrentLatitude, CurrentLongitude, distance, TotalDist;                         //saved long/lat
float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude);  // added
//float SavedLongitude= -74.070808; //american dream
//float SavedLatitude= 40.809204; //american dream
float SavedLongitude;     //= 0;
float SavedLatitude;      //= 0;
float Constant = 121.32;  //lines 53-61 from calc disc
// Create an instance of the TinyGPS object
TinyGPS gps;
// Initialize the NewSoftSerial library to the pins you defined above
SoftwareSerial uart_gps(RXPIN, TXPIN);
//-SoftwareSerial OpenLCD(6,7); // pin 7 = TX, pin 6 = RX (unused)

// This is where you declare prototypes for the functions that will be
// using the TinyGPS library.
void getgps(TinyGPS &gps);

//********************************Setup Loop*********************************//

void setup() 
{         //@setup
  delay(500);          // AnaLeah wait for display to boot up
  Serial.begin(9600);  // For debug use
  Serial.println("CAN Read - Testing receival of CAN Bus message");
  delay(1000);

  if (Canbus.init(CANSPEED_125))  //Initialise MCP2515 CAN controller at the specified speed
                                  //Changed from 500 -> 125 by Rebekah
                                  // Fuzail changed CANSPEED from 125 to 500,
    Serial.println("CAN Init ok");
  else
    Serial.println("Can't init CAN");

  delay(1000);
  Wire.begin();
  lcd.begin(Wire);
  while (!Serial) {
    // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.print("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.print("card initialized.");
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
//}
//}  //@setup
//Commented out by Alex, leads to errors

//********************************Main Loop*********************************//

void loop() {  //@void

  tCAN message;
  if (mcp2515_check_message()) {          //@1
    if (mcp2515_get_message(&message)) {  //@2
  
      Serial.print("ID: ");
      Serial.print(message.id, HEX);

      Serial.print(", ");
      Serial.print("Data: ");
      Serial.print(message.header.length, DEC);
      Serial.print("  ");  //Leah added space to seperate length from message
      for (int i = 0; i < message.header.length; i++) {
        Serial.print(message.data[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");


      ///////////prints to LCD

      Serial.print(message.id, HEX);
      Serial.print(" Id ");

      if (message.id == 0x03B)  ////AnaLeah IF ID=3B   Bob:Removed ";"
      {
        lcd.setCursor(0, 2);  //Fuzail added
        // delay(1000);
        lcd.write("current:");

        /// BobAna Leah capture current as decimal value and display on LCD

        int IDEC = 0;                //Bob initialize IDEC to zero
        for (int i = 0; i < 2; i++)  //AnaLeah removed 2 and changed to 1 so it reads one byte
        {
          Serial.print(message.data[i], HEX);
          Serial.print(" Hex ");

          // Serial.print(message.data[i],DEC);
          int MDATA = (message.data[i]);       ///LeahAna when we put a ",DEC" gives us constant DEC value of 10
          Serial.print((float)MDATA / 10, 1);  ///LeahAna added this to add the decimal place
          ///Serial.print(MDATA,DEC);
          Serial.print(" Dec ");

          // IDEC=IDEC+((message.data[i],DEC)* (pow(256, 1-i)));
          IDEC = IDEC + (MDATA * (pow(256, 1 - i)));  ///BOB changed "message.data[i]" to MDATA
          Serial.print(IDEC);
          Serial.print(" IDEC ");
        }

        lcd.print((float)IDEC / 10, 1);  // LeahAna crossed out line below; aded the float and put IDEC instead of MDATA

        Serial.println("Final");

        /// BobAna Leah capture voltage as decimal value and display on LCD
        lcd.setCursor(0, 3);
        lcd.write("voltage:");  //Bob display "voltage:" on second line

        for (int i = 3; i < 4; i++)  /// AnaLeah changed i=2 to i=3.. gets rid of leading zero

        {
          lcd.print(message.data[i], DEC);  //LeahAna changed HEX to DEC
        }
      }
      delay(1000);

      String dataString = "";
      File dataFile = SD.open("datalog.txt", FILE_WRITE);

      // if the file is available, write to it:
      if (dataFile) {
        //LeahAna write timeStamp to SD card
        int timeStamp = millis();
        int IDEC = 0;
        //write to uSD card: timestamp, current, voltage
        dataFile.print(timeStamp);
        dataFile.print(",");
        dataFile.print((float)IDEC / 10, 1);
        dataFile.print(",");
        int i = 3;
        dataFile.print(message.data[i], DEC);
      }
      dataFile.println();  //create a new row to read data more clearly
      dataFile.close();    //close file
      delay(100);          //LeahAna added a delay
      Serial.println();    //print to the serial port too:

    }

    // if the file isn't open, pop up an error:

    else {
      Serial.println("error opening datalog.txt");
    }

  }  //@2
}  //@1

//}  //@void //Commented out by Alex, leads to errors
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
    dataFile.print(gps.f_speed_kmph());

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
  lcd.print(gps.f_speed_kmph());
  // Here you can print the altitude and course values directly since
  // there is only one value for the function
  Serial.print("Altitude (meters): ");
  Serial.println(gps.f_altitude());
  // Same goes for course
  Serial.print("Course (degrees): ");
  Serial.println(gps.f_course());
  // And same goes for speed
  Serial.print("Speed (kmph): ");
  Serial.println(gps.f_speed_kmph());
  Serial.println();

  // Here you can print statistics on the sentences (not used here).
  // unsigned long chars;
  // unsigned short sentences, failed_checksum;
  // gps.stats(&chars, &sentences, &failed_checksum);
  //Serial.print("Failed Checksums: ");Serial.print(failed_checksum);
  //Serial.println(); Serial.println();
}

float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude) {

  // HaverSine version
  const float Deg2Rad = 0.01745329252;  // (PI/180)  0.017453293, 0.0174532925
  //const double EarthRadius = 6372.795;              //6372.7976 In Kilo meters, will scale to other values
  const double EarthRadius = 3961;  // In Miles 3956
  //const float EarthRadius = 20908120.1;              // In feet  20908128.6
  float DeltaLatitude, DeltaLongitude, a, Distance;
  if (SavedLongitude == 0 || SavedLatitude == 0) {
    SavedLongitude = CurrentLongitude;
    SavedLatitude = CurrentLatitude;
  }
  // degrees to radians
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
