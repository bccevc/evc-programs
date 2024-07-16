/****************************************************************************
CAN Read Demo for the SparkFun CAN Bus Shield. 

Written by Stephen McCoy. 
Original tutorial available here: http://www.instructables.com/id/CAN-Bus-Sniffing-and-Broadcasting-with-Arduino
Used with permission 2016. License CC By SA. 

Distributed as-is; no warranty is given.

Copied from SparkFun_SerialLCD_Demo
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
#include <SoftwareSerial.h>

const int chipSelect = 9;

SerLCD lcd;

void setup() {
  delay(500); // AnaLeah wait for display to boot up
  Serial.begin(9600); // For debug use
  Serial.println("CAN Read - Testing receival of CAN Bus message");  
  delay(1000);
  
  if (Canbus.init(CANSPEED_125)) {  //Initialise MCP2515 CAN controller at the specified speed
    Serial.println("CAN Init ok");
  }
  else {
    Serial.println("Can't init CAN");
  }
    
  delay(1000);
  Wire.begin();
  lcd.begin(Wire); 
  Serial.begin(9600); 
    
  Serial.print("Initializing SD card...");
  pinMode(chipSelect, OUTPUT); // Set default chip select pin to output even if not used
  if (!SD.begin(chipSelect)) {
    Serial.print("Card failed, or not present");
    return;
  }
  Serial.print("card initialized.");
}

void loop() {
  tCAN message;
  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {  
      Serial.print("ID: ");
      Serial.print(message.id,HEX);
      Serial.print(", ");
      Serial.print("Data: ");
      Serial.print(message.header.length,DEC);
      Serial.print("  "); //Leah added space to seperate length from message
      for (int i = 0; i < message.header.length; i++) {	
        Serial.print(message.data[i],HEX);
        Serial.print(" ");
      }
      Serial.println("");
             
      // Print to LCD
      Serial.print(message.id,HEX);
      Serial.print(" Id ");             
      if (message.id == 0x03B) {
        lcd.setCursor(0,2);
        lcd.write("current:");
        int IDEC = 0; //Bob initialize IDEC to zero
        for (int i = 0; i < 2; i++)  { //AnaLeah removed 2 and changed to 1 so it reads one byte
          Serial.print(message.data[i],HEX);
          Serial.print(" Hex "); 
          // Serial.print(message.data[i],DEC);
          int MDATA = (message.data[i]); // LeahAna when we put a ",DEC" gives us constant DEC value of 10
          Serial.print((float)MDATA/10,1); // LeahAna added this to add the decimal place
          ///Serial.print(MDATA,DEC); 
          Serial.print(" Dec "); 
          // IDEC=IDEC+((message.data[i],DEC)* (pow(256, 1-i)));
          IDEC=IDEC+(MDATA*(pow(256, 1-i))); // BOB changed "message.data[i]" to MDATA
          Serial.print(IDEC);
          Serial.print(" IDEC ");
        }
        lcd.print((float)IDEC/10,1); // LeahAna crossed out line below; aded the float and put IDEC instead of MDATA
        Serial.println("Final"); // Why?
        lcd.setCursor(0,3);  
        lcd.write("voltage:");
        for (int i = 3; i < 4; i++) { // AnaLeah changed i=2 to i=3.. gets rid of leading zero
          lcd.print(message.data[i],DEC);
        }
      }  
      delay(1000);

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
        delay(100);
        Serial.println();                         
      }
      else {
        Serial.println("error opening datalog.txt");
      }
    }
  }
  else {
    Serial.println("Error getting can message.");
  }
}