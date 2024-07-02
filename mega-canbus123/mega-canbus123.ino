/****************************************************************************
CAN Read Demo for the SparkFun CAN Bus Shield. 

Written by Stephen McCoy. 
Original tutorial available here: http://www.instructables.com/id/CAN-Bus-Sniffing-and-Broadcasting-with-Arduino
Used with permission 2016. License CC By SA. 

Distributed as-is; no warranty is given.

Some snippets are from SparkFun_SerialLCD_Demo
*************************************************************************/

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

SoftwareSerial uart_gps(RXPIN, TXPIN);
void getgps(TinyGPS &gps);
float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude);

const int chipSelect = 9;
float longitude,latitude,CurrentLatitude,CurrentLongitude,distance,TotalDist, Power,EnergyUsed,MPGe; //saved long/lat
float Constant = 121.32;
float SavedLongitude;
float SavedLatitude;
int timeStamp = millis();
int CurrentTS,timeElps, SavedTS;
float CEnergy=0;

SerLCD lcd;
TinyGPS gps;

void setup() {
  delay(500);
  Serial.begin(115200); // For debug use
  Serial.println("CAN Read - Testing receival of CAN Bus message");  
  delay(1000);
  
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
  // Make sure that the default chip select pin is set to output, even if you don't use it
  pinMode(chipSelect, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.print("Card failed, or not present");
    return;
  }
  Serial.print("card initialized.");
  // This is the serial rate for your terminal program. It must be this 
  // fast because we need to print everything before a new sentence 
  // comes in. If you slow it down, the messages might not be valid and 
  // you will likely get checksum errors.

  Serial3.begin(GPSBAUD);
  Serial.begin(115200);
  Wire.begin();
  lcd.begin(Wire);
  Wire.setClock(400000); // Optional - set I2C SCL to High Speed Mode of 400kHz
  lcd.clear();
  lcd.cursor(); //Turn on the underline cursor
 
  delay(500); // Wait for display to boot
  
  //Set GPS baud rate
  uart_gps.begin(GPSBAUD);
  
  Serial.println("");
  Serial.println("GPS Shield QuickStart Example Sketch v12");
  Serial.println("       ...waiting for lock...           ");
  Serial.println("");

  SavedTS = millis();
}

void loop() {
  int timeStamp = millis();
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

      // Print to LCD and write to SD
      Serial.print(message.id,HEX);
      Serial.print(" Id ");             
      if (message.id == 0x03B) {
        // Print current to LCD
        lcd.setCursor(0,2);
        lcd.write("current:");        
        int IDEC = 0; //Bob initialize IDEC to zero
        for (int i = 0; i < 2; i++) {
          Serial.print(message.data[i],HEX);
          Serial.print(" Hex "); 
          // Serial.print(message.data[i],DEC);
          int MDATA = (message.data[i]); ///LeahAna when we put a ",DEC" gives us constant DEC value of 10
          Serial.print((float)MDATA/10,1);///LeahAna added this to add the decimal place
          ///Serial.print(MDATA,DEC); 
          Serial.print(" Dec "); 
          // IDEC=IDEC+((message.data[i],DEC)* (pow(256, 1-i)));
          IDEC=IDEC+(MDATA*(pow(256, 1-i))); ///BOB changed "message.data[i]" to MDATA
          Serial.print(IDEC);
          Serial.print(" IDEC ");
        }
        lcd.print((float)IDEC/10,1); // LeahAna crossed out line below; aded the float and put IDEC instead of MDATA
        Serial.println("Final");
        
        // Print voltage to LCD
        lcd.setCursor(0,3);  
        lcd.write("voltage:"); //Bob display "voltage:" on second line
        for (int i = 3; i < 4; i++) { // Gets rid of leading zero
          lcd.print(message.data[i],DEC); //LeahAna changed HEX to DEC
        }
        delay(1000); // Remove if there's a second gap on timestamp?       
        
        // Write timestamp, current, voltage to SD
        String dataString = "";    
        File dataFile = SD.open("datalog.txt", FILE_WRITE);
        if (dataFile) {  
          int timeStamp = millis();
          int IDEC = 0;
          dataFile.print(timeStamp);
          dataFile.print(",");
          dataFile.print((float)IDEC/10,1);
          dataFile.print(",");
          int i=3;
          dataFile.print(message.data[i],DEC);
          dataFile.println();
          dataFile.close();
          delay(100); // Possibly remove if affects timestamp?
          Serial.println();                   
        }
        else {
          Serial.println("error opening datalog.txt");
        }
      }

      // Print battery temp, cell #, voltage to LCD
      int CDEC = 0; //Bob initialize IDEC to zero
      for (int i = 0; i < 2; i++) { // Removed 2 and changed to 1 so it reads one byte
        int MDATA = (message.data[i]); // LeahAna when we put a ",DEC" gives us constant DEC value of 10
        CDEC=CDEC+(MDATA*(pow(256, 1-i))); // BOB changed "message.data[i]" to MDATA
      }
      lcd.setCursor(10,1); 
      lcd.print("BT:");
      lcd.print(message.data[3],DEC);
      lcd.print("C");
      lcd.setCursor(0,1);
      lcd.print("Cell#");
      lcd.print(message.data[2],DEC);
      lcd.setCursor(7,3);
      lcd.print((float)CDEC/10000,3); // LeahAna crossed out line below; aded the float and put IDEC instead of MDATA
      lcd.print("v");

      // Write timestamp, cell #, voltage, battery temp to SD
      String dataString = "";    
      File dataFile = SD.open("datalog.txt", FILE_WRITE);
      if (dataFile) {  
        int timeStamp = millis();
        dataFile.print(timeStamp);
        dataFile.print(" ms");
        dataFile.print(", ");
        dataFile.print("Cell#");
        dataFile.print(message.data[2],DEC);
        dataFile.print("  ");
        dataFile.print((float)CDEC/10000,3); // LeahAna crossed out line below; aded the float and put IDEC instead of MDAT
        dataFile.print("v  ");
        dataFile.print("BT= ");
        dataFile.print(message.data[3],DEC);                        
        dataFile.println(); //create a new row to read data more clearly
        dataFile.close();   //close file
        Serial.println();   //print to the serial port too:
      }
      else {
        Serial.println("Error opening datalog.txt (123 message)");
      }
    }
  }
}


void getgps(TinyGPS &gps) {
  gps.f_get_position(&latitude, &longitude);
  // Print variables latitude and longitude:
  Serial.print("Lat/Long: "); 
  Serial.print(latitude,6); 
  Serial.print(", "); 
  Serial.println(longitude,6);
   // String for assembling the data to log:
  String dataString = "";
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {  
    int timeStamp = millis();
    //write to SD card
    dataFile.println();
    CurrentTS = timeStamp;
    dataFile.print(timeStamp);
    dataFile.print(" ms");
    dataFile.print(", ");
    //review Serial Monitor for debugging
    timeElps= CurrentTS-SavedTS; 
    Serial.print(" Timestamp: "); 
    Serial.print(timeStamp); 
    Serial.print(" CurrentTS: "); 
    Serial.print(CurrentTS); 
    Serial.print(" SavedTS: "); 
    Serial.print(SavedTS);  
    Serial.print(" TimeElapsed: "); 
    Serial.print(timeElps);
    EnergyUsed=(Power)*(timeElps);
    Serial.print(" Energy Used: ");
    Serial.print (EnergyUsed);
    dataFile.print(" Energy Used: ");
    dataFile.print (EnergyUsed);
    CEnergy= EnergyUsed + CEnergy;
    Serial.print(" Cumulative Energy: ");
    Serial.println (CEnergy);
    dataFile.print(" Cumulative Energy:");
    dataFile.print(CEnergy);
    if (CEnergy>0) { 
      MPGe = (TotalDist/CEnergy)*Constant;
      Serial.print(" C: ");
      Serial.println (Constant);
      Serial.print(" MPGe: ");
      Serial.println (MPGe,9);
      dataFile.print("MPGe:");
      dataFile.print(MPGe,9);
    }
    dataFile.print("Lat/Long: "); 
    dataFile.print(latitude,5); 
    dataFile.print(", ");
    dataFile.print(longitude,5);
    dataFile.print(" Trip:");
    dataFile.print(TotalDist);
    dataFile.print(" Distance:");
    dataFile.print(distance);
    int year;
    byte month, day, hour, minute, second, hundredths;
    gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
    // Print date
    dataFile.print(" Date: "); 
    dataFile.print(month, DEC);
    dataFile.print("/"); 
    dataFile.print(day, DEC);
    dataFile.print("/"); 
    dataFile.print(year);
    //Print time
    int echour = hour - 5;
    dataFile.print("  Time: "); 
    dataFile.print(echour, DEC); 
    dataFile.print(":"); 
    dataFile.print(minute, DEC); 
    dataFile.print(":"); 
    dataFile.print(second, DEC); 
    dataFile.print("."); 
    dataFile.print(hundredths, DEC);
    // Here you can print the altitude and course values directly since 
    // there is only one value for the function
    //dataFile.print("Altitude (meters): "); dataFile.println(gps.f_altitude());  
    // Same in course:
    dataFile.print(" Course (degrees): "); 
    dataFile.print(gps.f_course()); 
    // Same in speed:
    dataFile.print(" Speed(mph): "); 
    dataFile.print(gps.f_speed_mph());

    dataFile.println();
    dataFile.println();
    dataFile.close();
  } 
  //Some stuff repeats, supposed to for else function
  else { 
    Serial.println("error opening datalog.txt");
  } 

  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
  // Print date
  Serial.print("Date: "); 
  Serial.print(month, DEC); 
  Serial.print("/"); 
  Serial.print(day, DEC); 
  Serial.print("/"); 
  Serial.print(year);
  //Print time
  int echour = hour - 5;
  Serial.print("  Time: "); 
  Serial.print(echour, DEC); 
  Serial.print(":"); 
  Serial.print(minute, DEC); 
  Serial.print(":"); 
  Serial.print(second, DEC); 
  Serial.print("."); 
  Serial.println(hundredths, DEC);
  //Since month, day, hour, minute, second, and hundred
  //Functions for LCD
  lcd.setCursor(0,0);
  lcd.print(echour, DEC);
  lcd.print(":");
  lcd.print(minute, DEC);
  lcd.print(":");
  lcd.print(second, DEC);
  //lcd.print(":");
  //lcd.print(hundredths, DEC);
  lcd.print(" ");
  //display speed
  lcd.setCursor(0,1);
  //lcd.print("Speed:");
  lcd.print(gps.f_speed_mph());
  lcd.print("Mph ");
  lcd.setCursor(8,1);
  lcd.print(" ");
  // Here you can print the altitude and course values directly since 
  // there is only one value for the function
  // Serial.print("Altitude (meters): "); Serial.println(gps.f_altitude());  
  // Same in course
  Serial.print("Course (degrees): "); 
  Serial.println(gps.f_course()); 
  // And same goes for speed
  Serial.print("Speed(mph): "); 
  Serial.println(gps.f_speed_mph());
  // Serial.println();
}

/*----------------------------------------------------------------------**
**     Haversine Formula                                                **
** (from R. W. Sinnott, "Virtues of the Haversine," Sky and Telescope,  **
** vol. 68, no. 2, 1984, p. 159):                                       **
**                                                                      **
**   dLon = lon2 - lon1                                                 **
**   dLat = lat2 - lat1                                                 **
**   a = (sin(dlat/2))^2 + cos(lat1) * cos(lat2) * (sin(dlon/2))^2      **
**   c = 2 * atan2(sqrt(a), sqrt(1-a))                                  **
**   d = R * c                                                          **
**                                                                      **
** will give mathematically and computationally exact results. The      **
** intermediate result c is the great circle distance in radians. The   **
** great circle distance d will be in the same units as R.              **
**----------------------------------------------------------------------*/

float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude) {
  // HaverSine version
  const float Deg2Rad = 0.01745329252;               
    // (PI/180)  0.017453293, 0.0174532925
    //const double EarthRadius = 6372.795;              
    //6372.7976 In Kilo meters, will scale to other values
  const double EarthRadius = 3961;              
    // In Miles 3956
    //const float EarthRadius = 20908120.1;              
    // In feet  20908128.6
  float DeltaLatitude, DeltaLongitude, a, Distance;
  if (SavedLongitude==0 ||SavedLatitude==0) {
    SavedLongitude = CurrentLongitude;
    SavedLatitude = CurrentLatitude;  
  }
  // degrees to radians
  CurrentLatitude = (CurrentLatitude + 180) * Deg2Rad;     
  // Remove negative offset (0-360), convert to RADS
  CurrentLongitude = (CurrentLongitude + 180) * Deg2Rad;
  SavedLatitude = (SavedLatitude + 180) * Deg2Rad;
  SavedLongitude = (SavedLongitude + 180) * Deg2Rad;
  DeltaLatitude = SavedLatitude - CurrentLatitude;
  DeltaLongitude = SavedLongitude - CurrentLongitude;
  a =(sin(DeltaLatitude/2) * sin(DeltaLatitude/2)) + cos(CurrentLatitude) * cos(SavedLatitude) * (sin(DeltaLongitude/2) * sin(DeltaLongitude/2));
  Distance = EarthRadius * (2 * atan2(sqrt(a),sqrt(1-a)));
  return(Distance);
}
