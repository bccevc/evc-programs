# Connecting the Mega to the CAN-bus Shield

## Connecting the GPS to the Shield

- Attach a male end to the third wire from the left of the GPS cable and insert that into pin 14 of the Mega board.
- Attach a male end to the second wire from the left of the GPS cable and insert that into pin 15 of the Mega board.

## Making the SD Card Work

Use jumper wires to connect pins:

| CAN-bus | Mega |
|---|---|
|10|53|
|13|52|
|11|51|
|12|50|

# SparkFun LCD to CAN-bus Shield
- [Hookup guide](https://learn.sparkfun.com/tutorials/avr-based-serial-enabled-lcds-hookup-guide/introduction)

# Notes on Connecting to the CAN-bus
- When running programs, make sure the baud rates are the same as the CAN-bus via `CANSPEED_125`.