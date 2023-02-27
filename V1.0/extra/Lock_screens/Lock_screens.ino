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
#include "Lock_screens.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#define TFT_CS1         5
#define TFT_RST1        19
#define TFT_DC1         22
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS1, TFT_DC1, TFT_RST1);

void disp_centered_text(String t_disp, int y){
   int16_t x1, y1;
   uint16_t w, h;
   tft.getTextBounds(t_disp, 160, 0, &x1, &y1, &w, &h);
   tft.setCursor(80 - (w / 2), y);
   tft.print(t_disp);
}

void draw_lock_screen(byte lscreen){
  if (lscreen == 0){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Austin[i][j]);
      }
    }
  }

  if (lscreen == 1){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Dallas[i][j]);
      }
    }
  }

  if (lscreen == 2){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Dallas_1[i][j]);
      }
    }
  }

  if (lscreen == 3){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Dallas_2[i][j]);
      }
    }
  }

  if (lscreen == 4){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Dallas_3[i][j]);
      }
    }
  }

  if (lscreen == 5){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Denver[i][j]);
      }
    }
  }

  if (lscreen == 6){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Kuwait_City[i][j]);
      }
    }
  }

  if (lscreen == 7){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Miami[i][j]);
      }
    }
  }

  if (lscreen == 8){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Minneapolis[i][j]);
      }
    }
  }

  if (lscreen == 9){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Montreal[i][j]);
      }
    }
  }

  if (lscreen == 10){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Salt_Lake_City[i][j]);
      }
    }
  }

  if (lscreen == 11){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Singapore[i][j]);
      }
    }
  }

  if (lscreen == 12){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, St_Paul[i][j]);
      }
    }
  }

  if (lscreen == 13){
    for (int i = 0; i < 160; i++){
      for (int j = 0; j < 128; j++){
        tft.drawPixel(i, j, Tel_Aviv[i][j]);
      }
    }
  }
}

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(0x0000);
}

byte i;
void loop() {
  draw_lock_screen(i);
  disp_centered_text("Press Any Key", 115);
  delay(5000);
  i++;
  if (i > 13)
    i = 0;
}
