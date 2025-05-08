/******************************************************************************
 * EVC CAN Bus Sniffer & Decoder
 * 
 * 🛠️ Purpose:
 * This Arduino sketch reads raw CAN messages from the Orion BMS using the 
 * SparkFun CAN-BUS Shield and decodes selected message IDs for voltage, 
 * current, temperature, and cell-level diagnostics. Other messages are 
 * displayed in raw hex format with timestamps for debugging and filtering.
 * 
 * 👨‍💻 Written by:
 * Omer Kilic & Marek Solowiej
 * 
 * 🗓️ Last Updated: May 8, 2025
 * 
 * 📚 Dependencies:
 * - Canbus.h        (SparkFun CAN library)
 * - mcp2515.h       (Low-level MCP2515 support)
 * - SPI.h           (Arduino SPI core)
 * 
 * 🔧 Hardware Used:
 * - Arduino Mega 2560
 * - SparkFun CAN-BUS Shield
 * 
 * 📈 Output:
 * Serial Monitor @ 115200 baud — prints all CAN message traffic with decoded
 * details for critical BMS IDs and raw hex for all others.
 * 
 * 💡 Tip:
 * Use this sniffer to analyze background CAN traffic and optimize data handling
 * before deploying full logging/visualization code.
 ******************************************************************************/


// === Include Required Libraries ===
#include <Canbus.h>         // Library for CAN bus communication (SparkFun CAN shield)
#include <mcp2515.h>        // Handles low-level MCP2515 CAN controller functions
#include <mcp2515_defs.h>   // Definitions for MCP2515 registers and constants
#include <SPI.h>            // SPI library for communication with MCP2515 chip

// === Define a CAN message structure ===
tCAN message; // This holds incoming CAN message data: ID, length, and 8 bytes of data

// === Setup Function ===
void setup() {
  Serial.begin(9600);       // Start Serial Monitor at 9600 baud for output
  delay(1000);              // Small delay to allow serial monitor to initialize

  // Initialize the CAN bus at 125 kbps (typical for automotive applications)
  if (Canbus.init(CANSPEED_125)) {
    Serial.println("✅ CAN Bus Initialized");
  } else {
    Serial.println("❌ CAN Bus Init Failed");
    while (1); // Stop execution if CAN initialization fails
  }
}

// === Main Loop Function ===
void loop() {
  // Check if a new CAN message has been received
  if (mcp2515_check_message() && mcp2515_get_message(&message)) {

    // Use switch-case to handle different message IDs
    switch (message.id) {

      // ----------- Voltage & Current from CAN ID 0x03B -----------
      case 0x03B: {
        // Bytes 0 and 1 hold a signed 16-bit current value
        int16_t rawCurrent = (message.data[0] << 8) | message.data[1];  // Combine bytes
        float current = rawCurrent / 10.0;  // Convert to amps (divide by 10)

        // Byte 3 holds voltage as an integer (e.g., 130 = 13.0V)
        int16_t rawVoltage = (message.data[2] << 8) | message.data[3];  // Raw volts
        float voltage = rawVoltage / 1.00;
        // Voltage takes 2 bytes

        // Print decoded voltage and current
        Serial.print("BMD ID 0x03B 🔋 Voltage: ");
        Serial.print(voltage, 1);
        Serial.print(" V | ⚡ Current: ");
        Serial.print(current, 1);
        Serial.println(" A");

        Serial.print("BMD ID 0x03B Raw Bytes: ");
        for (int i = 0; i < 8; i++) {
          Serial.print("0x");
          if (message.data[i] < 0x10) Serial.print("0");
          Serial.print(message.data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();

        break;
      }

      // ----------- Low Cell Voltage & Temperature from CAN ID 0x123 -----------
      case 0x123: {
        // Bytes 0-1: Low cell voltage in millivolts (but likely 0.1 mV units)
        uint16_t rawLowCellV = (message.data[0] << 8) | message.data[1];
        float lowCellV = rawLowCellV / 10000.0;  // scaling factor

        // Byte 2: cell ID (e.g., 2 = Cell #2)
        int cellID = message.data[2];

        // === Decode High Cell Voltage ===
        int highCellID = message.data[4];
        uint16_t rawHighCellV = (message.data[5] << 8) | message.data[6];
        float highCellV = rawHighCellV / 10000.0;

        // Byte 3: maximum battery temperature (in Celsius)
        int tempC = message.data[3];

        // Print decoded data
        Serial.print("BMS ID 0x123: 📉 Low Cell V: ");
        Serial.print(lowCellV, 3);
        Serial.print(" V (Cell ");
        Serial.print(cellID);
        Serial.print(") | 🌡️ Max Temp: ");
        Serial.print(tempC);
        Serial.println(" °C");

        Serial.print("BMS ID 0x123: 📈 High Cell V: ");
        Serial.print(highCellV, 4);
        Serial.print(" V (Cell ");
        Serial.print(highCellID);
        Serial.println(")");

        // === Debug: Print Raw Data ===
        Serial.print("BMS ID 0x123 Raw Bytes: ");
        for (int i = 0; i < 8; i++) {
          Serial.print("0x");
          if (message.data[i] < 0x10) Serial.print("0");
          Serial.print(message.data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();

        break;
      }

      // ----------- Discharge Current Limit from CAN ID 0x3CB -----------
      case 0x3CB: {

        float DCL = message.data[0];


        // CCL: 16-bit from bytes 2 & 3 (little-endian)
        float CCL = message.data[6];

        Serial.print("BMS ID 0x3CB 🟢 Discharge Current Limit: ");
        Serial.print(DCL, 1);
        Serial.println(" A | 🔵 Charge Current Limit: ");
        Serial.print(CCL, 1);
        Serial.println(" A");
       
        Serial.print("BMS ID 0x3CB Raw Bytes: ");
        for (int i = 0; i < 8; i++) {
          Serial.print("0x");
          if (message.data[i] < 0x10) Serial.print("0");
          Serial.print(message.data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();


        break;
      }

      // ----------- Status Flags / Bitfields from CAN ID 0x6B0 -----------
      case 0x6B0: {
        // Extract SOC from Byte 1
        uint8_t rawSOC = message.data[1];
        float soc = rawSOC * 1.0;  // Convert to float percentage if needed
       
        // Display SOC
        Serial.print("BMS ID 0x6B0 🔋 Pack SOC: ");
        Serial.print(soc, 1);
        Serial.println(" %");

        // Display full raw status flags
        Serial.print("BMS ID 0x6B0 ⚠️ Status Flags: ");
        for (int i = 0; i < message.header.length; i++) {
          Serial.print("0x");
          if (message.data[i] < 0x10) Serial.print("0"); // pad with 0
          Serial.print(message.data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        break;
      }

      /*
      // ----------- All Other CAN Messages (Raw Hex Display) -----------
      default: {
        Serial.print("📡 CAN ID: 0x");
        Serial.print(message.id, HEX);
        Serial.print(" | Data: ");
        for (int i = 0; i < message.header.length; i++) {
          Serial.print("0x");
          if (message.data[i] < 0x10) Serial.print("0");
          Serial.print(message.data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        break;
      }
      */

    }
  }
}
