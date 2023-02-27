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
#include "FastLED.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <EEPROM.h>
#include "serpent.h"

int ver_sq[10] = {0x23,0x74,0xa7,0xc8,0xb3,0xb6,0x23,0xd1,0x0a,0xcf};
uint8_t serp_key[32] = {
0xf0,0x7e,0x1a,0xb4,
0xab,0xa0,0x7f,0x2b,
0xdc,0xd9,0x63,0x8d,
0x3a,0x5d,0xd0,0xc1,
0x53,0xa4,0xdd,0x5c,
0x38,0xae,0xeb,0xfc,
0x25,0x5c,0x42,0xd9,
0xb4,0x58,0x9e,0xfa
};

#define LED_COUNT 32          // Number of LEDs in the strip
#define LED_DT 0             // Connect the stripe's DIN pin to the ESP8266's D3 pin
#define EEPROM_SIZE 2

byte ballColors[3][3] = {
  {0xff, 0, 0},
  {0xff, 0xff, 0xff},
  {0   , 0   , 0xff},
};

int s_red;
int s_green;
int s_blue;
int md = 30;
int ledMode;

int BOTTOM_INDEX = 0;
int TOP_INDEX = int(LED_COUNT / 2);
int EVENODD = LED_COUNT % 2;
struct CRGB leds[LED_COUNT];
int ledsX[LED_COUNT][3];     //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, MARCH, ETC)

int thisdelay = 20;          //-FX LOOPS DELAY VAR
int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR

int thisindex = 0;
int thisRED = 0;
int thisGRN = 0;
int thisBLU = 0;

int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR

void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i].setRGB( cred, cgrn, cblu);
  }
}

void one_color_allHSV(int ahue) {    //-SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i] = CHSV(ahue, thissat, 255);
  }
}

typedef struct struct_message {
  char encr_with_serp[16];
} struct_message;

struct_message myData;

int getNum(char ch)
{
    int num=0;
    if(ch>='0' && ch<='9')
    {
        num=ch-0x30;
    }
    else
    {
        switch(ch)
        {
            case 'A': case 'a': num=10; break;
            case 'B': case 'b': num=11; break;
            case 'C': case 'c': num=12; break;
            case 'D': case 'd': num=13; break;
            case 'E': case 'e': num=14; break;
            case 'F': case 'f': num=15; break;
            default: num=0;
        }
    }
    return num;
}

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  decr_Serpent(myData.encr_with_serp);
}

void decr_Serpent(char res[]){
      uint8_t ct1[32], pt1[32], key[64];
      int plen, clen, i, j;
      serpent_key skey;
      serpent_blk ct2;
      uint32_t *p;
  
  for (i=0; i<1; i++) {
    hex2bin (key);
  
    // set key
    memset (&skey, 0, sizeof (skey));
    p=(uint32_t*)&skey.x[0][0];
    
    serpent_setkey (&skey, key);
    //Serial.printf ("\nkey=");

    for (j=0; j<sizeof(skey)/sizeof(serpent_subkey_t)*4; j++) {
      if ((j % 8)==0) putchar('\n');
      //Serial.printf ("%08X ", p[j]);
    }

    for(int i = 0; i <16; i++)
      ct2.b[i] = res[i];
    /*
    Serial.printf ("\n\n");
    for(int i = 0; i<16; i++){
    Serial.printf("%x", ct2.b[i]);
    Serial.printf(" ");
    */
    }
    //Serial.printf("\n");
    serpent_encrypt (ct2.b, &skey, SERPENT_DECRYPT);
    /*
    for (int i=0; i<16; i++) {
      Serial.print(int(ct2.b[i]));
      Serial.print(" ");
    }
    Serial.println();
    */
    bool sqnc = true;
    
    for (int i = 0; i < 10; i++){
      if (ct2.b[i] != ver_sq[i])
        sqnc = false;
    }
    
    if (sqnc == true){
      if (ct2.b[14] == 0){
        md = 0;
        change_mode(md);
      }
      if (ct2.b[14] == 1){
        EEPROM.begin(EEPROM_SIZE);
        LEDS.setBrightness(EEPROM.read(1));
        EEPROM.end();
        if (ct2.b[15] == 44 || ct2.b[15] == 1)
          LEDS.setBrightness(255);
        md = ct2.b[15];
        change_mode(md);
      }
      if (ct2.b[14] == 10){ // Set Brightness
        EEPROM.begin(EEPROM_SIZE);
        EEPROM.write(1, int(ct2.b[15]));
        LEDS.setBrightness(EEPROM.read(1));
        EEPROM.end();
      }
        
    }
    else{
      Serial.println("Failed to verify the authenticity of the received packet!!!");
    }
}

size_t hex2bin(void * bin) {
  size_t len, i;
  int x;
  uint8_t * p = (uint8_t * ) bin;
  for (i = 0; i < 32; i++) {
    p[i] = (uint8_t) serp_key[i];
  }
  return 32;
}

void change_mode(int newmode) {
  thissat = 255;
  switch (newmode) {
    case 0: one_color_all(0, 0, 0); LEDS.show(); break; //---ALL OFF
    case 1: one_color_all(255, 255, 255); LEDS.show(); break; //---ALL ON
    case 2: thisdelay = 20; break;                      //---STRIP RAINBOW FADE
    case 3: thisdelay = 20; thisstep = 10; break;       //---RAINBOW LOOP
    case 4: thisdelay = 20; break;                      //---RANDOM BURST
    case 5: thisdelay = 20; thishue = 0; break;         //---CYLON v1
    case 6: thisdelay = 40; thishue = 0; break;         //---CYLON v2
    case 7: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SINGLE
    case 8: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SOLID
    case 9: thishue = 160; thissat = 50; break;         //---STRIP FLICKER
    case 10: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR BRIGHTNESS
    case 11: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR SATURATION
    case 12: thisdelay = 60; thishue = 180; break;      //---VERTICAL SOMETHING
    case 13: thisdelay = 100; break;                    //---CELL AUTO - RULE 30 (RED)
    case 14: thisdelay = 40; break;                     //---MARCH RANDOM COLORS
    case 15: thisdelay = 80; break;                     //---MARCH RWB COLORS
    case 16: thisdelay = 60; thishue = 95; break;       //---RADIATION SYMBOL
    //---PLACEHOLDER FOR COLOR LOOP VAR DELAY VARS
    case 19: thisdelay = 35; thishue = 180; break;      //---SIN WAVE BRIGHTNESS
    case 20: thisdelay = 100; thishue = 0; break;       //---POP LEFT/RIGHT
    case 21: thisdelay = 100; thishue = 180; break;     //---QUADRATIC BRIGHTNESS CURVE
    //---PLACEHOLDER FOR FLAME VARS
    case 23: thisdelay = 50; thisstep = 15; break;      //---VERITCAL RAINBOW
    case 24: thisdelay = 50; break;                     //---PACMAN
    case 25: thisdelay = 35; break;                     //---RANDOM COLOR POP
    case 26: thisdelay = 25; thishue = 0; break;        //---EMERGECNY STROBE
    case 27: thisdelay = 25; thishue = 0; break;        //---RGB PROPELLER
    case 28: thisdelay = 100; thishue = 0; break;       //---KITT
    case 29: thisdelay = 50; thishue = 95; break;       //---MATRIX RAIN
    case 30: thisdelay = 5; break;                      //---NEW RAINBOW LOOP
    case 31: thisdelay = 100; break;                    //---MARCH STRIP NOW CCW
    case 32: thisdelay = 100; break;                    //---MARCH STRIP NOW CCW
    case 33: thisdelay = 50; break;                     // colorWipe
    case 34: thisdelay = 50; break;                     // CylonBounce
    case 35: thisdelay = 15; break;                     // Fire
    case 36: thisdelay = 50; break;                     // NewKITT
    case 37: thisdelay = 20; break;                     // rainbowCycle
    case 38: thisdelay = 10; break;                     // rainbowTwinkle
    case 39: thisdelay = 50; break;                     // RunningLights
    case 40: thisdelay = 0; break;                      // Sparkle
    case 41: thisdelay = 20; break;                     // SnowSparkle
    case 42: thisdelay = 50; break;                     // theaterChase
    case 43: thisdelay = 50; break;                     // theaterChaseRainbow
    case 44: thisdelay = 100; break;                    // Strobe
    case 50: one_color_all(s_red, s_green, s_blue); LEDS.show(); break; //---CUSTOM COLOR
  }
  bouncedirection = 0;
  one_color_all(0, 0, 0);
  ledMode = newmode;
}

void switch_mode(int ledMode){
  switch (ledMode) {
    case 999: break;
    case  2: rainbow_fade(); break;
    case  3: rainbow_loop(); break;
    case  4: random_burst(); break;
    case  5: color_bounce(); break;
    case  6: color_bounceFADE(); break;
    case  7: ems_lightsONE(); break;
    case  8: ems_lightsALL(); break;
    case  9: flicker(); break;
    case 10: pulse_one_color_all(); break;
    case 11: pulse_one_color_all_rev(); break;
    case 12: fade_vertical(); break;
    case 13: rule30(); break;
    case 14: random_march(); break;
    case 15: rwb_march(); break;
    case 16: radiation(); break;
    case 17: color_loop_vardelay(); break;
    case 18: white_temps(); break;
    case 19: sin_bright_wave(); break;
    case 20: pop_horizontal(); break;
    case 21: quad_bright_curve(); break;
    case 22: flame(); break;
    case 23: rainbow_vertical(); break;
    case 24: pacman(); break;
    case 25: random_color_pop(); break;
    case 26: ems_lightsSTROBE(); break;
    case 27: rgb_propeller(); break;
    case 28: kitt(); break;
    case 29: matrix(); break;
    case 30: new_rainbow_loop(); break;
    case 31: strip_march_ccw(); break;
    case 32: strip_march_cw(); break;
    case 33: colorWipe(0x00, 0xff, 0x00, thisdelay);
      colorWipe(0x00, 0x00, 0x00, thisdelay); break;
    case 34: CylonBounce(0xff, 0, 0, 4, 10, thisdelay); break;
    case 35: Fire(55, 120, thisdelay); break;
    case 36: NewKITT(0xff, 0, 0, 8, 10, thisdelay); break;
    case 37: rainbowCycle(thisdelay); break;
    case 39: RunningLights(0xff, 0xff, 0x00, thisdelay); break;
    case 40: Sparkle(0xff, 0xff, 0xff, thisdelay); break;
    case 41: SnowSparkle(0x10, 0x10, 0x10, thisdelay, random(100, 1000)); break;
    case 42: theaterChase(0xff, 0, 0, thisdelay); break;
    case 43: theaterChaseRainbow(thisdelay); break;
    case 44: Strobe(0xff, 0xff, 0xff, 10, thisdelay, 1000); break;

    case 45: BouncingBalls(0xff, 0, 0, 3); break;
    case 46: BouncingColoredBalls(3, ballColors); break;

  }
}
 
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
  EEPROM.begin(EEPROM_SIZE);
    if (EEPROM.read(0) == 255){
      EEPROM.write(0, 240);
      EEPROM.write(1, 63);
    }
  EEPROM.end();

  EEPROM.begin(EEPROM_SIZE);
    LEDS.setBrightness(EEPROM.read(1));
  EEPROM.end();
  

  LEDS.addLeds<WS2811, LED_DT, GRB>(leds, LED_COUNT);
  one_color_all(0, 0, 0);
  LEDS.show(); 
}

void loop() {
  switch_mode(md);
}
