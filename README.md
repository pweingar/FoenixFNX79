# Foenix KBD-79 Firmware

This project builds the firmware for the Foenix KDB-79 PS/2 keyboard from Foenix Retro Systems.
It should be built using the ST CubeIDE.

## Special Commands

The FNX-KBD79 has three additional PS/2 commands beyond the standard set.
These commands allow a program to control the color and status of any of the RGB LEDs.
Each command takes one to four additional bytes in the PS/2 stream to provide the
parameters for the command.

| Command | Parameter bytes             | Function                         |
| ------- | --------------------------- | -------------------------------- |
| 0xe0    | LED_INDEX, RED, GREEN, BLUE | Set the "on" color of an RGB LED |
| 0xe1    | LED_INDEX                   | Turn an RGB LED on               |
| 0xe2    | LED_INDEX                   | Turn an RGB LED off              | 

Each RGB LED has a unique LED index:

| LED Index | Purpose   |
| --------- | --------- |
| 0         | Power LED |
| 1         | Media     |
| 2         | Network   |
| 3         | CAPS Lock |
| 4         | F1        |
| 5         | F2        |
| 6         | F3        |
| 7         | F4        |
| 8         | F5        |
| 9         | F6        |
| 10        | F7        |
| 11        | F8        |

## Special Key Scancodes

For the moment, at least, MON, APP, and MENU keys are assigned to work like the function keys F9, F10, and F11 respectively.

| Key           | Scan Code Set 1 | Scan Code Set 2 |
| ------------- | --------------- | --------------- |
| MON           | 0x43            | 0x01            |
| APP LIST      | 0x44            | 0x09            |
| MENU HELP     | 0x57            | 0x78            |
| Left "blank"  | 0x5a            | 0x13            |
| Right "blank" | 0x29            | 0x53            |

## To Do

1. Adjust timing logic for PS/2 commands being sent from the host to the keyboard to better support variations from different computer's
   implementation of the protocol.
1. Adjust logic for the production board (when available).
1. Create final production version of the firmware.

## Acknowledgements

* Low-level PS/2 code was modified from the ps2dev library for Arduino: https://github.com/harvie/ps2dev.
* WS2812 (neopixel) code was based on examples provided here: https://www.thevfdcollective.com/blog/stm32-and-sk6812-rgbw-led
