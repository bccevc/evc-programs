// In order for this sketch to work, you will need to download 
// TinyGPS library from arduiniana.org and put them 
// into the hardware->libraries folder in your ardiuno directory.

#include <Wire.h>
#include <SoftwareWire.h>
#include <TinyGPS.h>
TinyGPS gps;
#define DISPLAY_ADDRESS1 0x72 //This is the default address of the OpenWire
// Define which pins you will use on the Arduino to communicate with your 
// GPS. In this case, the GPS module's TX pin will connect to the 
// Arduino's RXPIN which is pin 3.
#define RXPIN 4
#define TXPIN 5
//Set this value equal to the baud rate of your GPS
#define GPSBAUD 4800

// Create an instance of the TinyGPS object
// Initialize the NewSoftWire library to the pins you defined above
SoftwareWire uart_gps(RXPIN, TXPIN);


void getgps(TinyGPS &gps);
int cycles = 0;

void setup() {
  Wire.begin(); //Join the bus as master

  //By default .begin() will set I2C SCL to Standard Speed mode of 100kHz
  //Wire.setClock(400000); //Optional - set I2C SCL to High Speed Mode of 400kHz

  Wire.begin(9600); //Start Wire communication at 9600 for debug statements
  Serial.begin(9600); //Start serial communication at 9600 for debug statements
  Wire.setClock(400000);//added this  //Optional - set I2C SCL to High Speed Mode of 400kHz
  uart_gps.begin(GPSBAUD);
  Serial.println("OpenLCD Example Code");
  Serial.println("GPS Info Using I2C");
  //Send the reset command to the display - this forces the cursor to return to the beginning of the display
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write('|'); //Put Wire into setting mode
  Wire.write('-'); //Send clear display command
  Wire.endTransmission();
   
  //program stuff for the gps initialization
  Serial.println("");
  Serial.println("GPS Shield QuickStart Example Sketch v12");
  Serial.println("       ...waiting for lock...           ");
  Serial.println("");
}



void loop()
{
  while(uart_gps.available())     // While there is data on the RX pin...
  {
      int c = uart_gps.read();    // load the data into a variable...
      if(gps.encode(c))      // if there is a new valid sentence...
      {
        getgps(gps);         // then grab the data.
      }
  }
  delay(50);
 // i2cSendValue(cycles);
}



//function for the gps stuff
void getgps(TinyGPS &gps)
{
  // To get all of the data into varialbes that you can use in your code, 
  // all you need to do is define variables and query the object for the 
  // data. To see the complete list of functions see keywords.txt file in 
  // the TinyGPS and NewSoftWire libs.
  
  // Define the variables that will be used
  float latitude, longitude;
  // Then call this function
  gps.f_get_position(&latitude, &longitude);
  // You can now print variables latitude and longitude
  Wire.print("Lat/Long: "); 
  Wire.print(latitude,5); 
  Wire.print(", "); 
  Serial.println(longitude,5);
  
  // Same goes for date and time
  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
  // Print data and time
  Wire.print("Date: "); Wire.print(month, DEC); Wire.print("/"); 
  Wire.print(day, DEC); Wire.print("/"); Wire.print(year);
  Wire.print("  Time: "); Wire.print(hour, DEC); Wire.print(":"); 
  Wire.print(minute, DEC); Wire.print(":"); Wire.print(second, DEC); 
  Wire.print("."); Serial.println(hundredths, DEC);
  //Since month, day, hour, minute, second, and hundr
  
  // Here you can print the altitude and course values directly since 
  // there is only one value for the function
  Wire.print("Altitude (meters): "); Serial.println(gps.f_altitude());  
  // Same goes for course
  Wire.print("Course (degrees): "); Serial.println(gps.f_course()); 
  // And same goes for speed
  Wire.print("Speed(kmph): "); Serial.println(gps.f_speed_kmph());
  Serial.println();
  
  // Here you can print statistics on the sentences.
  unsigned long chars;
  unsigned short sentences, failed_checksum;
  gps.stats(&chars, &sentences, &failed_checksum);
  //Wire.print("Failed Checksums: ");Wire.print(failed_checksum);
  //Serial.println(); Serial.println();
  Wire.beginTransmission(DISPLAY_ADDRESS1); // transmit to device #1
 // Wire.setCursor(0,0);
 // Wire.print(echour, DEC);
  Wire.print(":");
  Wire.print(minute, DEC);
  Wire.print(":");
  Wire.print(second, DEC);

  Wire.print(" ");
  //display speed
  //Wire.setCursor(0,1);
  //Wire.print("Speed:");
  Wire.print(gps.f_speed_mph());
  Wire.print("Mph ");
 // Wire.setCursor(8,1); //....
  Wire.print(" ");  // ....
  Wire.endTransmission(); //Stop I2C transmission

}

void i2cSendValue(int value)
{
  Serial.println("Is this working?");
  Wire.beginTransmission(DISPLAY_ADDRESS1); // transmit to device #1

  Wire.write('|'); //Put Wire into setting mode
  Wire.write('-'); //Send clear display command

  Wire.print("This is working");
  
  Wire.endTransmission(); //Stop I2C transmission
}