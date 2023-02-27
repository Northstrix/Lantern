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
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#define TFT_CS1         5
#define TFT_RST1        19
#define TFT_DC1         22
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS1, TFT_DC1, TFT_RST1);

#include <Keypad.h>
#define ROW_NUM     4
#define COLUMN_NUM  4

char p_k[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pin_rows[ROW_NUM]      = {13, 12, 14, 27};
byte pin_column[COLUMN_NUM] = {26, 25, 33, 32};
Keypad keypad = Keypad( makeKeymap(p_k), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

void disp_centered_text(String t_disp, int y){
   int16_t x1, y1;
   uint16_t w, h;
   tft.getTextBounds(t_disp, 160, 0, &x1, &y1, &w, &h);
   tft.setCursor(80 - (w / 2), y);
   tft.print(t_disp);
}

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  disp_centered_text("Lantern", 5);
  tft.setCursor(0, 20);
}

void loop() {
    char key = keypad.getKey();
    if (key) {
     tft.print(key);
   }
   delayMicroseconds(400);
}
