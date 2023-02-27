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

uint16_t c;
String keyboard_input;
int menu_pos;
const uint16_t current_inact_clr = 0x051b;
bool finish_input;
bool act;

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

void disp_menu(){
  tft.setTextSize(1);
  if (menu_pos == 0){
    tft.fillScreen(0x0000);
    tft.setTextColor(0xffff);
    disp_centered_text("Rainbow Fade", 9);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Loop", 24);
    disp_centered_text("Rainbow Burst", 39);
    disp_centered_text("Smooth Rainbow", 54);
    disp_centered_text("Pinball", 69);
    disp_centered_text("Soft Pinball", 84);
    disp_centered_text("2PX Police Lights", 99);
    disp_centered_text("Circular Police Lights", 114);
  }
  if (menu_pos == 1){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Fade", 9);
    tft.setTextColor(0xffff);
    disp_centered_text("Rainbow Loop", 24);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Burst", 39);
    disp_centered_text("Smooth Rainbow", 54);
    disp_centered_text("Pinball", 69);
    disp_centered_text("Soft Pinball", 84);
    disp_centered_text("2PX Police Lights", 99);
    disp_centered_text("Circular Police Lights", 114);
  }
  if (menu_pos == 2){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Fade", 9);
    disp_centered_text("Rainbow Loop", 24);
    tft.setTextColor(0xffff);
    disp_centered_text("Rainbow Burst", 39);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Smooth Rainbow", 54);
    disp_centered_text("Pinball", 69);
    disp_centered_text("Soft Pinball", 84);
    disp_centered_text("2PX Police Lights", 99);
    disp_centered_text("Circular Police Lights", 114);
  }
  if (menu_pos == 3){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Fade", 9);
    disp_centered_text("Rainbow Loop", 24);
    disp_centered_text("Rainbow Burst", 39);
    tft.setTextColor(0xffff);
    disp_centered_text("Smooth Rainbow", 54);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Pinball", 69);
    disp_centered_text("Soft Pinball", 84);
    disp_centered_text("2PX Police Lights", 99);
    disp_centered_text("Circular Police Lights", 114);
  }
  if (menu_pos == 4){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Fade", 9);
    disp_centered_text("Rainbow Loop", 24);
    disp_centered_text("Rainbow Burst", 39);
    disp_centered_text("Smooth Rainbow", 54);
    tft.setTextColor(0xffff);
    disp_centered_text("Pinball", 69);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Soft Pinball", 84);
    disp_centered_text("2PX Police Lights", 99);
    disp_centered_text("Circular Police Lights", 114);
  }
  if (menu_pos == 5){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Fade", 9);
    disp_centered_text("Rainbow Loop", 24);
    disp_centered_text("Rainbow Burst", 39);
    disp_centered_text("Smooth Rainbow", 54);
    disp_centered_text("Pinball", 69);
    tft.setTextColor(0xffff);
    disp_centered_text("Soft Pinball", 84);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("2PX Police Lights", 99);
    disp_centered_text("Circular Police Lights", 114);
  }
  if (menu_pos == 6){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Fade", 9);
    disp_centered_text("Rainbow Loop", 24);
    disp_centered_text("Rainbow Burst", 39);
    disp_centered_text("Smooth Rainbow", 54);
    disp_centered_text("Pinball", 69);
    disp_centered_text("Soft Pinball", 84);
    tft.setTextColor(0xffff);
    disp_centered_text("2PX Police Lights", 99);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Circular Police Lights", 114);
  }
  if (menu_pos == 7){
    tft.fillScreen(0x0000);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Rainbow Fade", 9);
    disp_centered_text("Rainbow Loop", 24);
    disp_centered_text("Rainbow Burst", 39);
    disp_centered_text("Smooth Rainbow", 54);
    disp_centered_text("Pinball", 69);
    disp_centered_text("Soft Pinball", 84);
    disp_centered_text("2PX Police Lights", 99);
    tft.setTextColor(0xffff);
    disp_centered_text("Circular Police Lights", 114);
  }
  if (menu_pos == 8){
    tft.fillScreen(0x0000);
    tft.setTextColor(0xffff);
    disp_centered_text("Flicker", 9);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Breathing Red", 24);
    disp_centered_text("Red To White", 39);
    disp_centered_text("Weird Blue", 54);
    disp_centered_text("Weird Red", 69);
    disp_centered_text("Color March", 84);
    disp_centered_text("American Carnival", 99);
    disp_centered_text("Breathing Green", 114);
  }
  if (menu_pos == 9){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Flicker", 9);
    tft.setTextColor(0xffff);
    disp_centered_text("Breathing Red", 24);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Red To White", 39);
    disp_centered_text("Weird Blue", 54);
    disp_centered_text("Weird Red", 69);
    disp_centered_text("Color March", 84);
    disp_centered_text("American Carnival", 99);
    disp_centered_text("Breathing Green", 114);
  }
  if (menu_pos == 10){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Flicker", 9);
    disp_centered_text("Breathing Red", 24);
    tft.setTextColor(0xffff);
    disp_centered_text("Red To White", 39);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Weird Blue", 54);
    disp_centered_text("Weird Red", 69);
    disp_centered_text("Color March", 84);
    disp_centered_text("American Carnival", 99);
    disp_centered_text("Breathing Green", 114);
  }
  if (menu_pos == 11){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Flicker", 9);
    disp_centered_text("Breathing Red", 24);
    disp_centered_text("Red To White", 39);
    tft.setTextColor(0xffff);
    disp_centered_text("Weird Blue", 54);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Weird Red", 69);
    disp_centered_text("Color March", 84);
    disp_centered_text("American Carnival", 99);
    disp_centered_text("Breathing Green", 114);
  }
  if (menu_pos == 12){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Flicker", 9);
    disp_centered_text("Breathing Red", 24);
    disp_centered_text("Red To White", 39);
    disp_centered_text("Weird Blue", 54);
    tft.setTextColor(0xffff);
    disp_centered_text("Weird Red", 69);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Color March", 84);
    disp_centered_text("American Carnival", 99);
    disp_centered_text("Breathing Green", 114);
  }
  if (menu_pos == 13){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Flicker", 9);
    disp_centered_text("Breathing Red", 24);
    disp_centered_text("Red To White", 39);
    disp_centered_text("Weird Blue", 54);
    disp_centered_text("Weird Red", 69);
    tft.setTextColor(0xffff);
    disp_centered_text("Color March", 84);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("American Carnival", 99);
    disp_centered_text("Breathing Green", 114);
  }
  if (menu_pos == 14){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Flicker", 9);
    disp_centered_text("Breathing Red", 24);
    disp_centered_text("Red To White", 39);
    disp_centered_text("Weird Blue", 54);
    disp_centered_text("Weird Red", 69);
    disp_centered_text("Color March", 84);
    tft.setTextColor(0xffff);
    disp_centered_text("American Carnival", 99);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Breathing Green", 114);
  }
  if (menu_pos == 15){
    tft.fillScreen(0x0000);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Flicker", 9);
    disp_centered_text("Breathing Red", 24);
    disp_centered_text("Red To White", 39);
    disp_centered_text("Weird Blue", 54);
    disp_centered_text("Weird Red", 69);
    disp_centered_text("Color March", 84);
    disp_centered_text("American Carnival", 99);
    tft.setTextColor(0xffff);
    disp_centered_text("Breathing Green", 114);
  }
  if (menu_pos == 16){
    tft.fillScreen(0x0000);
    tft.setTextColor(0xffff);
    disp_centered_text("Running Red", 9);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Center Blue", 24);
    disp_centered_text("Fire", 39);
    disp_centered_text("Another Rainbow", 54);
    disp_centered_text("Shooting Stars", 69);
    disp_centered_text("Emergency Strobe", 84);
    disp_centered_text("Circular Run", 99);
    disp_centered_text("Jumping Red", 114);
  }
  if (menu_pos == 17){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Running Red", 9);
    tft.setTextColor(0xffff);
    disp_centered_text("Center Blue", 24);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Fire", 39);
    disp_centered_text("Another Rainbow", 54);
    disp_centered_text("Shooting Stars", 69);
    disp_centered_text("Emergency Strobe", 84);
    disp_centered_text("Circular Run", 99);
    disp_centered_text("Jumping Red", 114);
  }
  if (menu_pos == 18){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Running Red", 9);
    disp_centered_text("Center Blue", 24);
    tft.setTextColor(0xffff);
    disp_centered_text("Fire", 39);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Another Rainbow", 54);
    disp_centered_text("Shooting Stars", 69);
    disp_centered_text("Emergency Strobe", 84);
    disp_centered_text("Circular Run", 99);
    disp_centered_text("Jumping Red", 114);
  }
  if (menu_pos == 19){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Running Red", 9);
    disp_centered_text("Center Blue", 24);
    disp_centered_text("Fire", 39);
    tft.setTextColor(0xffff);
    disp_centered_text("Another Rainbow", 54);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Shooting Stars", 69);
    disp_centered_text("Emergency Strobe", 84);
    disp_centered_text("Circular Run", 99);
    disp_centered_text("Jumping Red", 114);
  }
  if (menu_pos == 20){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Running Red", 9);
    disp_centered_text("Center Blue", 24);
    disp_centered_text("Fire", 39);
    disp_centered_text("Another Rainbow", 54);
    tft.setTextColor(0xffff);
    disp_centered_text("Shooting Stars", 69);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Emergency Strobe", 84);
    disp_centered_text("Circular Run", 99);
    disp_centered_text("Jumping Red", 114);
  }
  if (menu_pos == 21){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Running Red", 9);
    disp_centered_text("Center Blue", 24);
    disp_centered_text("Fire", 39);
    disp_centered_text("Another Rainbow", 54);
    disp_centered_text("Shooting Stars", 69);
    tft.setTextColor(0xffff);
    disp_centered_text("Emergency Strobe", 84);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Circular Run", 99);
    disp_centered_text("Jumping Red", 114);
  }
  if (menu_pos == 22){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Running Red", 9);
    disp_centered_text("Center Blue", 24);
    disp_centered_text("Fire", 39);
    disp_centered_text("Another Rainbow", 54);
    disp_centered_text("Shooting Stars", 69);
    disp_centered_text("Emergency Strobe", 84);
    tft.setTextColor(0xffff);
    disp_centered_text("Circular Run", 99);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Jumping Red", 114);
  }
  if (menu_pos == 23){
    tft.fillScreen(0x0000);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Running Red", 9);
    disp_centered_text("Center Blue", 24);
    disp_centered_text("Fire", 39);
    disp_centered_text("Another Rainbow", 54);
    disp_centered_text("Shooting Stars", 69);
    disp_centered_text("Emergency Strobe", 84);
    disp_centered_text("Circular Run", 99);
    tft.setTextColor(0xffff);
    disp_centered_text("Jumping Red", 114);
  }
  if (menu_pos == 24){
    tft.fillScreen(0x0000);
    tft.setTextColor(0xffff);
    disp_centered_text("Ascending Green", 9);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Green Ring", 24);
    disp_centered_text("Fast Red", 39);
    disp_centered_text("Weird Fire", 54);
    disp_centered_text("Running Lights", 69);
    disp_centered_text("Theater Chase", 84);
    disp_centered_text("Strobing Moon", 99);
    disp_centered_text("Shining Moon", 114);
  }
  if (menu_pos == 25){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Ascending Green", 9);
    tft.setTextColor(0xffff);
    disp_centered_text("Green Ring", 24);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Fast Red", 39);
    disp_centered_text("Weird Fire", 54);
    disp_centered_text("Running Lights", 69);
    disp_centered_text("Theater Chase", 84);
    disp_centered_text("Strobing Moon", 99);
    disp_centered_text("Shining Moon", 114);
  }
  if (menu_pos == 26){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Ascending Green", 9);
    disp_centered_text("Green Ring", 24);
    tft.setTextColor(0xffff);
    disp_centered_text("Fast Red", 39);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Weird Fire", 54);
    disp_centered_text("Running Lights", 69);
    disp_centered_text("Theater Chase", 84);
    disp_centered_text("Strobing Moon", 99);
    disp_centered_text("Shining Moon", 114);
  }
  if (menu_pos == 27){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Ascending Green", 9);
    disp_centered_text("Green Ring", 24);
    disp_centered_text("Fast Red", 39);
    tft.setTextColor(0xffff);
    disp_centered_text("Weird Fire", 54);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Running Lights", 69);
    disp_centered_text("Theater Chase", 84);
    disp_centered_text("Strobing Moon", 99);
    disp_centered_text("Shining Moon", 114);
  }
  if (menu_pos == 28){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Ascending Green", 9);
    disp_centered_text("Green Ring", 24);
    disp_centered_text("Fast Red", 39);
    disp_centered_text("Weird Fire", 54);
    tft.setTextColor(0xffff);
    disp_centered_text("Running Lights", 69);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Theater Chase", 84);
    disp_centered_text("Strobing Moon", 99);
    disp_centered_text("Shining Moon", 114);
  }
  if (menu_pos == 29){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Ascending Green", 9);
    disp_centered_text("Green Ring", 24);
    disp_centered_text("Fast Red", 39);
    disp_centered_text("Weird Fire", 54);
    disp_centered_text("Running Lights", 69);
    tft.setTextColor(0xffff);
    disp_centered_text("Theater Chase", 84);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Strobing Moon", 99);
    disp_centered_text("Shining Moon", 114);
  }
  if (menu_pos == 30){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Ascending Green", 9);
    disp_centered_text("Green Ring", 24);
    disp_centered_text("Fast Red", 39);
    disp_centered_text("Weird Fire", 54);
    disp_centered_text("Running Lights", 69);
    disp_centered_text("Theater Chase", 84);
    tft.setTextColor(0xffff);
    disp_centered_text("Strobing Moon", 99);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Shining Moon", 114);
  }
  if (menu_pos == 31){
    tft.fillScreen(0x0000);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Ascending Green", 9);
    disp_centered_text("Green Ring", 24);
    disp_centered_text("Fast Red", 39);
    disp_centered_text("Weird Fire", 54);
    disp_centered_text("Running Lights", 69);
    disp_centered_text("Theater Chase", 84);
    disp_centered_text("Strobing Moon", 99);
    tft.setTextColor(0xffff);
    disp_centered_text("Shining Moon", 114);
  }
  if (menu_pos == 32){
    tft.fillScreen(0x0000);
    tft.setTextColor(0xffff);
    disp_centered_text("Lamp Mode", 9);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Custom Color", 24);
    disp_centered_text("OFF", 39);
    disp_centered_text("Set Brightness", 54);
  }
  if (menu_pos == 33){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Lamp Mode", 9);
    tft.setTextColor(0xffff);
    disp_centered_text("Custom Color", 24);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("OFF", 39);
    disp_centered_text("Set Brightness", 54);
  }
  if (menu_pos == 34){
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Lamp Mode", 9);
    disp_centered_text("Custom Color", 24);
    tft.setTextColor(0xffff);
    disp_centered_text("OFF", 39);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Set Brightness", 54);
  }
  if (menu_pos == 35){
    tft.fillScreen(0x0000);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Lamp Mode", 9);
    disp_centered_text("Custom Color", 24);
    disp_centered_text("OFF", 39);
    tft.setTextColor(0xffff);
    disp_centered_text("Set Brightness", 54);
  }
}

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  menu_pos = 0;
  disp_menu();
}

void loop() {
   char key = keypad.getKey();
    if (key) {
      
      if (key == '8') {
        menu_pos--;
      }

      else if (key == '0'){
        menu_pos++;
      }

      if (menu_pos > 35)
        menu_pos = 0;

      if (menu_pos < 0)
        menu_pos = 35;
        
      disp_menu();
    }
}
