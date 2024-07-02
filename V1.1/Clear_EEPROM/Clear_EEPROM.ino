/*
Lantern
Distributed under the MIT License
© Copyright Maxim Bortnikov 2024
For more information please visit
https://github.com/Northstrix/Lantern
https://sourceforge.net/projects/the-lantern-project/
Required libraries:
TFT_eSPI: https://github.com/Bodmer/TFT_eSPI
FastLED: https://github.com/FastLED/FastLED
serpent: https://github.com/peterferrie/serpent
NintendoExtensionCtrl: https://github.com/dmadison/NintendoExtensionCtrl

Effects are taken from https://alexgyver.ru/ws2812b-fx/
The soil sprite is taken from https://www.kaaringaming.com/platformer-tutorial
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
