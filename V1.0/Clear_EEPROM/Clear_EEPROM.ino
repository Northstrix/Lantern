/*
Lantern
Distributed under the MIT License
© Copyright Maxim Bortnikov 2023
For more information please visit
https://github.com/Northstrix/Lantern
https://sourceforge.net/projects/the-lantern-project/
https://osdn.net/projects/lantern/
Required libraries:
Adafruit-GFX-Library: https://github.com/adafruit/Adafruit-GFX-Library
Adafruit_BusIO: https://github.com/adafruit/Adafruit_BusIO
Adafruit-ST7735-Library: https://github.com/adafruit/Adafruit-ST7735-Library
Keypad: https://github.com/Chris--A/Keypad
FastLED: https://github.com/FastLED/FastLED
serpent: https://github.com/peterferrie/serpent

Effects were taken from https://alexgyver.ru/ws2812b-fx/
*/
#include <EEPROM.h>
#define EEPROM_SIZE 2

void setup() {
    EEPROM.begin(EEPROM_SIZE);
    for (int i = 0; i < EEPROM_SIZE; i++){
      EEPROM.write(i, 255);
    }
    EEPROM.end();
}

void loop() {
}
