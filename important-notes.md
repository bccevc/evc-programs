# Mega and CAN-bus Shield Connections

## Connecting the GPS to the Shield

- Attach a male end to the third wire (aka the middle wire) from the left of the GPS cable and insert that into pin 14 of the Mega board.
- Attach a male end to the second wire (the remaining loose wire) from the left of the GPS cable and insert that into pin 15 of the Mega board.

## Making the SD Card Work

Use jumper wires to connect pins:

| CAN-bus | Mega |
|---|---|
|10|53|
|13|52|
|11|51|
|12|50|

## Connecting the LCD to the Shield

- Reference this [Hookup guide](https://learn.sparkfun.com/tutorials/avr-based-serial-enabled-lcds-hookup-guide/introduction)
    - This is the [hookup guide for I2C](https://cdn.sparkfun.com/assets/learn_tutorials/7/8/9/Fritzing_Arduino_SerLCD_I2C_bb.jpg)

| LCD | Breadboard | CAN-bus |
|---|---|---|
|GND|A27||
|RAW|A15||
|SDA|D30 (LV1 on the Logic Level Converter)||
|SCL|D29 (LV2)||
||E16 to -2 (from top right)||
||D28 (LV)|3.3v|
||H28 (HV)|5v|
||-1 (top right)|GND|
||E15|VIN|
||H30 (HV1)|SDA|
||H29 (HV2)|SCL|
||J27 to -27 (from the right)||

# Notes on Connecting to the CAN-bus
- When running programs, make sure the baud rates are the same as the CAN-bus via `CANSPEED_125`.

# Truck Metrics
- Voltage
- Current
- Time
- Speed (kmph)
- Lat/Long
- Altitude (meters)
- Course (degrees)
- RPM
- Trip