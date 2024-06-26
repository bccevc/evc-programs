/****************************************************************************
CAN Read Demo for the SparkFun CAN Bus Shield. 

Written by Stephen McCoy. 
Original tutorial available here: http://www.instructables.com/id/CAN-Bus-Sniffing-and-Broadcasting-with-Arduino
Used with permission 2016. License CC By SA. 

Distributed as-is; no warranty is given.
*************************************************************************/

#include <Canbus.h>
#include <defaults.h>//changed from defaultsMega.h
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>

#include <SPI.h>
#include <SD.h>
const int chipSelect = 9;

#include <Wire.h>
#include <SerLCD.h>
/////////////////copied from SparkFun_SerialLCD_Demo/////////

#include <SoftwareSerial.h> 

SerLCD lcd;

//********************************Setup Loop*********************************//

void setup() { //@setup
  delay(500); // AnaLeah wait for display to boot up
  Serial.begin(9600); // For debug use
  Serial.println("CAN Read - Testing receival of CAN Bus message");  
  delay(1000);
  
  if(Canbus.init(CANSPEED_125))  //Initialise MCP2515 CAN controller at the specified speed
  //Changed from 500 -> 125 by Rebekah
                                 // Fuzail changed CANSPEED from 125 to 500, 
    Serial.println("CAN Init ok");
  else
    Serial.println("Can't init CAN");
    
   delay(1000);
   Wire.begin();
   lcd.begin(Wire); 
    Serial.begin(9600); 
        while (!Serial) 
        {
          ; // wait for serial port to connect. Needed for Leonardo only
        }
    
        Serial.print("Initializing SD card...");
// make sure that the default chip select pin is set to
// output, even if you don't use it:
        pinMode(chipSelect, OUTPUT);

// see if the card is present and can be initialized:
         if (!SD.begin(chipSelect)) 
         {
            Serial.print("Card failed, or not present");
            // don't do anything more:
            return;
         }
      Serial.print("card initialized.");
} //@setup

//********************************Main Loop*********************************//

void loop(){ //@void

{

  tCAN message;
if (mcp2515_check_message()) 
  { //@1
    if (mcp2515_get_message(&message)) 
  { //@2       
               Serial.print("ID: ");
               Serial.print(message.id,HEX);
        
               Serial.print(", ");
               Serial.print("Data: ");
               Serial.print(message.header.length,DEC);
               Serial.print("  "); //Leah added space to seperate length from message
               for(int i=0;i<message.header.length;i++) 
                {	
                  Serial.print(message.data[i],HEX);
                  Serial.print(" ");
                }
               Serial.println("");
             

///////////prints to LCD 

                Serial.print(message.id,HEX);
                Serial.print(" Id "); 
                 
                                
                if (message.id == 0x03B)    ////AnaLeah IF ID=3B   Bob:Removed ";"
                  {
                   lcd.setCursor(0,2); //Fuzail added
                   // delay(1000);
                   lcd.write("current:");
                  
    /// BobAna Leah capture current as decimal value and display on LCD
                    
                    int IDEC = 0; //Bob initialize IDEC to zero
                    for(int i=0;i<2;i++) //AnaLeah removed 2 and changed to 1 so it reads one byte
                      {
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
                
     /// BobAna Leah capture voltage as decimal value and display on LCD
                    lcd.setCursor(0,3);  
                    lcd.write("voltage:"); //Bob display "voltage:" on second line
                    
                    for(int i=3;i<4;i++)/// AnaLeah changed i=2 to i=3.. gets rid of leading zero
                  
                {  
                    lcd.print(message.data[i],DEC); //LeahAna changed HEX to DEC
                    
                }   
                   
                }      
                 delay(1000);               
                  
                   String dataString = "";    
                   File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
                   if (dataFile)   
                     {  
                        //LeahAna write timeStamp to SD card
                        int timeStamp = millis();
                        int IDEC = 0;
                        //write to uSD card
                        // timestamp, current, voltage
                        dataFile.print(timeStamp);
                        dataFile.print(",");
                        dataFile.print((float)IDEC/10,1);
                        dataFile.print(",");
                        int i=3;
                        dataFile.print(message.data[i],DEC);                        
                    }
    dataFile.println(); //create a new row to read data more clearly
    dataFile.close();   //close file
    delay(100);//LeahAna added a delay
    Serial.println();   //print to the serial port too:

  }
    
  // if the file isn't open, pop up an error:
    
    else
    {
      Serial.println("error opening datalog.txt");
    } 

  } //@2
  } //@1

} //@void
