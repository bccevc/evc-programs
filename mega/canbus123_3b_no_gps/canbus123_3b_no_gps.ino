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
float longitude,latitude,CurrentLatitude,CurrentLongitude,distance,TotalDist,Power, MPGe; //saved long/lat//energy used///power
float Constant = 121.32;

SoftwareSerial uart_gps(RXPIN, TXPIN);
void getgps(TinyGPS &gps);
float calcDist(float CurrentLatitude, float CurrentLongitude, float SavedLatitude, float SavedLongitude);

const int chipSelect = 9;
float SavedLongitude;
float SavedLatitude;
unsigned long timeStamp = millis(); 
//change to micros test
//int timeStamp = micros();
//unsigned long CurrentTS,timeElps, SavedTS;
float CurrentTS,timeElps, SavedTS; //we need the decimals -Alex
double CEnergy ;//changed to int //changed again to double
double EnergyUsed;//added
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
  //change to micros test
  //SavedTS = micros();
}

void loop() {
 unsigned long timeStamp = millis();  

  // Even if you aren't using GPS, you need to include this while loop if you've included the GPS function

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
            //print amps to display
            lcd.print((float)IDEC/10,1); // LeahAna crossed out line below; aded the float and put IDEC instead of MDATA
            lcd.print("A"); 
            Serial.println("Final");
            
            // Print voltage to LCD
            lcd.setCursor(7,2);  
            for (int i = 3; i < 4; i++) { // Gets rid of leading zero
              lcd.print(message.data[i],DEC); //LeahAna changed HEX to DEC
              lcd.print("V"); 
              //Power= (message.data[3],DEC)*((float)IDEC/10); // Voltage * Amps
              //changed [3] to [i] and removed DEC by Alex
              //                      V   V
              //Power= ((message.data[i],DEC)*((float)IDEC/10));
              //             ampsV   message.data[i]
              Power= (message.data[i])*((float)IDEC/10);
              // Voltage * Amps
              //Alex removed DEC from voltage, fixes error
  
              Serial.print("Amps:");
              Serial.print((float)IDEC/10,1);
              Serial.print("  Volts:");
              Serial.print(message.data[i],DEC);
              //added lines 176-179 to test power result 
              Serial.print(" Power:");
              Serial.print(Power);//
            }
            //  delay(1000); // Remove if there's a second gap on timestamp?       
            
            // Write timestamp, current, voltage to SD
            String dataString = "";    
            File dataFile = SD.open("datalog.txt", FILE_WRITE);
            if (dataFile) {  
               unsigned long timeStamp = millis(); 
              //change to micros test
              //int timeStamp = micros();

              int IDEC = 0;
              dataFile.print(timeStamp);
              dataFile.print(",");
              dataFile.print((float)IDEC/10,1);
              dataFile.print(",");
              // int i=3;
              dataFile.print(message.data[3],DEC); //originally calling i
              dataFile.close();
              delay(100); // Possibly remove if affects timestamp?
              Serial.println();                   
            }
            else {
              Serial.println("error opening datalog.txt");
            }
          }
          //added below line since the 0x3B message doesn't show every iteration -Alex    
          // Print battery temp, cell #, voltage to LCD
          if (message.id == 0x123) {
            int CDEC = 0; //Bob initialize IDEC to zero
            for (int i = 0; i < 2; i++) { // Removed 2 and changed to 1 so it reads one byte
              int MDATA = (message.data[i]); // LeahAna when we put a ",DEC" gives us constant DEC value of 10
              CDEC = CDEC + (MDATA*(pow(256, 1-i))); // BOB changed "message.data[i]" to MDATA
            }
            lcd.setCursor(0,1); 
            lcd.print("BT:");
            lcd.print(message.data[3],DEC);
            lcd.print("C");
            lcd.setCursor(17,1);
            lcd.print("#");
            lcd.print(message.data[2],DEC);
            lcd.setCursor(13,2);
            lcd.print((float)CDEC/10000,3);
            lcd.print("v");

            // Write timestamp, cell #, voltage, battery temp to SD
            String dataString = "";    
            File dataFile = SD.open("datalog.txt", FILE_WRITE);
            if (dataFile) {  
              unsigned long timeStamp = millis(); 
              long double  timeStampHr;
              long double  timeStampmin;
              long double  timeStampsec;
              //write to SD card
              dataFile.println();
              CurrentTS = timeStamp;
              timeElps = CurrentTS - SavedTS;
              timeElps = timeElps/1000; //ms to s
              timeElps = timeElps/60;   //s to m
              timeElps = timeElps/60;   //m to hr
              dataFile.print(timeStamp);
              dataFile.print(" ms");
              dataFile.print(", ");
              //review Serial Monitor for debugging
              Serial.print(" Timestamp: "); 
              Serial.print(timeStamp); 
              Serial.print(" CurrentTS: "); 
              Serial.print(CurrentTS); 
              Serial.print(" SavedTS: "); 
              Serial.print(SavedTS);  
              Serial.print(" TimeElapsed: "); 
              Serial.println(timeElps,4);
              SavedTS=CurrentTS;
              EnergyUsed=(Power)*(timeElps);
              EnergyUsed= EnergyUsed/1000;
              //need to divided by thousand for kwh
              Serial.print(" Energy Used: ");
              Serial.print (EnergyUsed,6);
              dataFile.print(" Energy Used: ");
              dataFile.print (EnergyUsed,6);
              CEnergy= (EnergyUsed) + CEnergy;
              Serial.print(" Cumulative Energy: ");
              Serial.println (CEnergy,4);
              dataFile.print(" Cumulative Energy:");
              dataFile.println(CEnergy,4);
              if (CEnergy > 0) { 
                MPGe = (TotalDist/CEnergy)*Constant;
                Serial.print(" C: ");
                Serial.println (Constant);
                Serial.print(" MPGe: ");
                Serial.println (MPGe,9);
                dataFile.print("MPGe:");
                dataFile.print(MPGe,9);
              }
    
                //change to micros test
                //int timeStamp = micros();
                dataFile.print(timeStamp);
                dataFile.print(",");
                dataFile.print(message.data[2],DEC);
                dataFile.print(",");
                dataFile.print((float)CDEC/10000,3);
                dataFile.print(",");
                dataFile.print(message.data[3],DEC);                 
                dataFile.println();
                dataFile.println();
                dataFile.close();
                Serial.println();
                lcd.setCursor(0,3);
                lcd.print(CEnergy,3);
                lcd.print("KWH"); 
                lcd.setCursor(9,0);
                lcd.print("RPM:____");
                lcd.setCursor(8,1);
                lcd.print("MC:___C"); 
                
            }
          }
        }
      } 
      else {
        Serial.println("Error getting canbus message.");
        }
}




