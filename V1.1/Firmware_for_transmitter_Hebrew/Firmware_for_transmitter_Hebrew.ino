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
#define FS_NO_GLOBALS
#include <FS.h>
#include "SPIFFS.h"
#include <math.h>
#include "pngle.h"
#include "soil_sprite.h"
#include "custom_hebrew_font.h"
#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include "serpent.h"
#include <NintendoExtensionCtrl.h>

#define USE_LINE_BUFFER  // Enable for faster rendering
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library
TFT_eSprite catSprite = TFT_eSprite(&tft);
TFT_eSprite hitCatSprite = TFT_eSprite(&tft);
TFT_eSprite sunflowerSprite = TFT_eSprite(&tft);
TFT_eSprite defaultHandSprite = TFT_eSprite(&tft);
TFT_eSprite attackHandSprite = TFT_eSprite(&tft);
Nunchuk wii_nunchuk;

#define NUNCHUCK_SDA 22
#define NUNCHUCK_SCL 27

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

uint8_t broadcastAddress[] = {0x5C, 0xCF, 0x7F, 0xFD, 0x85, 0x1D};
bool invert_y_axis = false;
#define SHOW_STATUS_CAT_FOR 300
byte threshold = 16;
int wait_till_fast_scroll = 500;
int delay_for_fast_scroll = 50;
#define COMMON_FILL_COLOR 0x4d7a
#define OFFSET_CAT_BY_X 2
#define OFFSET_CAT_BY_Y 112
const int MENU_ITEMS_PER_PAGE = 3;
const int MENU_ITEM_SPACING = 46;
const int MENU_TEXT_SIZE = 2; // irrelevant
/*
String menu_items[] = {
    "Rainbow Fade", "Rainbow Loop", "Rainbow Burst", "Smooth Rainbow",
    "Pinball", "Soft Pinball", "2PX Police Lights", "Circular Police Lights",
    "Flicker", "Breathing Red", "White To Red", "Weird Blue",
    "Weird Red", "Color March", "American Carnival", "Breathing Green",
    "Running Red", "Center Blue", "Fire", "Another Rainbow",
    "Shooting Stars", "Emergency Strobe", "Circular Run", "Jumping Red",
    "Ascending Green", "Green Ring", "Fast Red", "Weird Fire",
    "Running Lights", "Theater Chase", "Strobing Moon", "Shining Moon",
    "OFF", "Set Brightness"
};
*/

String menu_items[] = {
    "LiLAh hkST", "kST diih", "'rik r'Xh", "kST cLKh",
    "piNbiL A'", "piNbiL b'", "hMStrh AiriT A'", "hMStrh AiriT b'",
    "pL'kr", "AdiMh NS'Mh", "Adim LLbn", "CciL Mizr",
    "krNbL hAMr'kA'", "Sdh hhiLNd'", "Adim Mizr", "'rikh NS'Mh",
    "AdiMh r'Xh", "Mtrh", "CiCb'm NipL'm", "AMbiLNs",
    "AS", "AdiM 'rik CciL", "aid kST", "ric AdiMh",
    "hTprXiT", "tbaT h'rik", "Adim hMhir", "L''ts hT'Atrin",
    "sipT ciL", "AS hMizr", "stribisCip", "'rc MLA",
    "Cbi'", "br'kis kba"
};
int menu_shift_down = 3; // Pixels to shift menu items down
#define SUNFLOWER_X 253
#define SUNFLOWER_Y 91

int total_menu_items = sizeof(menu_items) / sizeof(menu_items[0]);
String string_for_ciphertext;
int menu_pos;
#define DEFAULT_TEXT_COLOR 0xef7d
#define DEFAULT_HIGHLIGHT_COLOR 0x2124
#define DEFAULT_RED_COLOR 0xf800

esp_now_peer_info_t peerInfo;

bool pressed_c = false;
bool pressed_z = false;
bool held_left = false;
bool held_up = false;
bool held_right = false;
bool held_down = false;
bool c_functions_as_enter = true;
bool right_fast_scroll = false;
bool left_fast_scroll = false;
bool up_fast_scroll = false;
bool down_fast_scroll = false;
bool do_right_fast_scroll = false;
bool do_left_fast_scroll = false;
bool do_up_fast_scroll = false;
bool do_down_fast_scroll = false;
unsigned long new_time_millis = 0;
bool colorSwap = false;
bool nunchukWasDisconnected = true;

typedef struct struct_message {
  char encr_with_serp[16];
} struct_message;

struct_message myData;

// Define constants for hand positions and sizes
const int DEFAULT_HAND_X = 48;
const int DEFAULT_HAND_Y = 14;
const int DEFAULT_HAND_WIDTH = 26;
const int DEFAULT_HAND_HEIGHT = 57;

const int ATTACK_HAND_X = 48;
const int ATTACK_HAND_Y = 18;
const int ATTACK_HAND_WIDTH = 38;
const int ATTACK_HAND_HEIGHT = 53;
int brightness = 127;

#define LINE_BUF_SIZE 240
uint16_t lbuf[LINE_BUF_SIZE];
int16_t sx = 0;
int16_t sy = 0;
uint32_t pc = 0;

int16_t png_dx = 0, png_dy = 0;

bool packet_delivery_status;

TFT_eSprite* currentSprite = nullptr;

void setPngPosition(int16_t x, int16_t y)
{
  png_dx = x;
  png_dy = y;
}

void pngle_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
  uint16_t color = (rgba[0] << 8 & DEFAULT_RED_COLOR) | (rgba[1] << 3 & 0x07e0) | (rgba[2] >> 3 & 0x001f);

  if (rgba[3] > 127) { // Transparency threshold
    if (currentSprite) {
      currentSprite->drawPixel(png_dx + (int32_t)x, png_dy + (int32_t)y, color);
    } else {
      tft.drawPixel(png_dx + (int32_t)x, png_dy + (int32_t)y, color);
    }
  } else {
    // If the pixel is transparent, don't draw it
    if (currentSprite) {
      currentSprite->drawPixel(png_dx + (int32_t)x, png_dy + (int32_t)y, TFT_TRANSPARENT);
    } else {
      tft.drawPixel(png_dx + (int32_t)x, png_dy + (int32_t)y, TFT_TRANSPARENT);
    }
  }
}

byte get_nunchuk_input() {
  boolean success = wii_nunchuk.update();

  unsigned long currentTime = millis();
  while (!success) {
    nunchukWasDisconnected = true;
    unsigned long currentTime = millis();

    // Check if 1 second has passed
    if (currentTime - new_time_millis >= 1000) {
      colorSwap = !colorSwap;
      new_time_millis = currentTime;

      if (colorSwap) {
        display_centered_highlighted_text("Nunchuk", 166, DEFAULT_TEXT_COLOR, DEFAULT_RED_COLOR);
        display_centered_highlighted_text("Disconnected", 186, DEFAULT_TEXT_COLOR, DEFAULT_RED_COLOR);
      } else {
        display_centered_highlighted_text("Nunchuk", 166, DEFAULT_RED_COLOR, DEFAULT_TEXT_COLOR);
        display_centered_highlighted_text("Disconnected", 186, DEFAULT_RED_COLOR, DEFAULT_TEXT_COLOR);
      }
    }

    delay(24);
    success = wii_nunchuk.update();
  }

  if (nunchukWasDisconnected == true) {
    display_centered_highlighted_text("Nunchuk", 166, COMMON_FILL_COLOR, COMMON_FILL_COLOR);
    display_centered_highlighted_text("Disconnected", 186, COMMON_FILL_COLOR, COMMON_FILL_COLOR);
    nunchukWasDisconnected = false;
  }

  // Check C button
  if (wii_nunchuk.buttonC()) {
    if (!pressed_c) {
      pressed_c = true;
      return c_functions_as_enter ? 13 : 27; // Return Enter (13) or Esc (27)
    }
  } else {
    pressed_c = false;
  }

  // Check Z button
  if (wii_nunchuk.buttonZ()) {
    if (!pressed_z) {
      pressed_z = true;
      return c_functions_as_enter ? 27 : 13; // Return Esc (27) or Enter (13)
    }
  } else {
    pressed_z = false;
  }

  byte XAxis = wii_nunchuk.joyX();
  byte YAxis = wii_nunchuk.joyY();

  // Check joystick right
  if (XAxis > (255 - threshold)) {
    if (held_right) {
      if (!right_fast_scroll) {
        do_right_fast_scroll = true;
        for (int i = 0; i < wait_till_fast_scroll; i++) {
          success = wii_nunchuk.update();
          delay(1);
          if (wii_nunchuk.joyX() < (255 - threshold)) {
            do_right_fast_scroll = false;
            break;
          }
        }
        if (do_right_fast_scroll) right_fast_scroll = true;
      }
      if (right_fast_scroll) {
        delay(delay_for_fast_scroll);
        return 130; // Right arrow (fast scroll)
      }
    }
    if (!held_right) {
      held_right = true;
      return 130; // Right arrow
    }
  } else {
    right_fast_scroll = false;
    held_right = false;
  }

  // Check joystick left
  if (XAxis < threshold) {
    if (held_left) {
      if (!left_fast_scroll) {
        do_left_fast_scroll = true;
        for (int i = 0; i < wait_till_fast_scroll; i++) {
          success = wii_nunchuk.update();
          delay(1);
          if (wii_nunchuk.joyX() > threshold) {
            do_left_fast_scroll = false;
            break;
          }
        }
        if (do_left_fast_scroll) left_fast_scroll = true;
      }
      if (left_fast_scroll) {
        delay(delay_for_fast_scroll);
        return 129; // Left arrow (fast scroll)
      }
    }
    if (!held_left) {
      held_left = true;
      return 129; // Left arrow
    }
  } else {
    left_fast_scroll = false;
    held_left = false;
  }

  // Check joystick up
  if (YAxis > (255 - threshold)) {
    if (held_up) {
      if (!up_fast_scroll) {
        do_up_fast_scroll = true;
        for (int i = 0; i < wait_till_fast_scroll; i++) {
          success = wii_nunchuk.update();
          delay(1);
          if (wii_nunchuk.joyY() < (255 - threshold)) {
            do_up_fast_scroll = false;
            break;
          }
        }
        if (do_up_fast_scroll) up_fast_scroll = true;
      }
      if (up_fast_scroll) {
        delay(delay_for_fast_scroll);
        return invert_y_axis ? 132 : 131; // Up or Down arrow (fast scroll)
      }
    }
    if (!held_up) {
      held_up = true;
      return invert_y_axis ? 132 : 131; // Up or Down arrow
    }
  } else {
    up_fast_scroll = false;
    held_up = false;
  }

  // Check joystick down
  if (YAxis < threshold) {
    if (held_down) {
      if (!down_fast_scroll) {
        do_down_fast_scroll = true;
        for (int i = 0; i < wait_till_fast_scroll; i++) {
          success = wii_nunchuk.update();
          delay(1);
          if (wii_nunchuk.joyY() > threshold) {
            do_down_fast_scroll = false;
            break;
          }
        }
        if (do_down_fast_scroll) down_fast_scroll = true;
      }
      if (down_fast_scroll) {
        delay(delay_for_fast_scroll);
        return invert_y_axis ? 131 : 132; // Down or Up arrow (fast scroll)
      }
    }
    if (!held_down) {
      held_down = true;
      return invert_y_axis ? 131 : 132; // Down or Up arrow
    }
  } else {
    down_fast_scroll = false;
    held_down = false;
  }

  return 0; // No input detected
}

void send_smt() {
  //Serial.println(menu_pos);
  packet_delivery_status = false;
  for (int i = 0; i < 24; i++) {
    if (packet_delivery_status == true)
      break;
  if (menu_pos == 0) // Rainbow Loop
    send_data_to_rec(1, 3);
  if (menu_pos == 1) // Rainbow Fade
    send_data_to_rec(1, 2);
  if (menu_pos == 2) // Ascending Green
    send_data_to_rec(1, 29);
  if (menu_pos == 3) // Smooth Rainbow
    send_data_to_rec(1, 30);
  if (menu_pos == 4) // Pinball
    send_data_to_rec(1, 5);
  if (menu_pos == 5) // Soft Pinball
    send_data_to_rec(1, 6);
  if (menu_pos == 6) // 2PX Police Lights
    send_data_to_rec(1, 7);
  if (menu_pos == 7) // Circular Police Lights
    send_data_to_rec(1, 8);
  if (menu_pos == 8) // Flicker
    send_data_to_rec(1, 9);
  if (menu_pos == 9) // Breathing Red
    send_data_to_rec(1, 10);
  if (menu_pos == 10) // White To Red
    send_data_to_rec(1, 11);
  if (menu_pos == 11) // Weird Blue
    send_data_to_rec(1, 12);
  if (menu_pos == 12) // American Carnival
    send_data_to_rec(1, 15);
  if (menu_pos == 13) // Color March
    send_data_to_rec(1, 14);
  if (menu_pos == 14) // Weird Red
    send_data_to_rec(1, 13);
  if (menu_pos == 15) // Breathing Green
    send_data_to_rec(1, 16);
  if (menu_pos == 16) // Running Red
    send_data_to_rec(1, 20);
  if (menu_pos == 17) // Center Blue
    send_data_to_rec(1, 21);
  if (menu_pos == 18) // Shooting Stars
    send_data_to_rec(1, 25);
  if (menu_pos == 19) // Emergency Strobe
    send_data_to_rec(1, 26);
  if (menu_pos == 20) // Fire
    send_data_to_rec(1, 22);
  if (menu_pos == 21) // Circular Run
    send_data_to_rec(1, 27);
  if (menu_pos == 22) // Another Rainbow
    send_data_to_rec(1, 23);
  if (menu_pos == 23) // Jumping Red
    send_data_to_rec(1, 28);
  if (menu_pos == 24) // Rainbow Burst
    send_data_to_rec(1, 4);
  if (menu_pos == 25) // Green Ring
    send_data_to_rec(1, 33);
  if (menu_pos == 26) // Fast Red
    send_data_to_rec(1, 34);
  if (menu_pos == 27) // Theater Chase
    send_data_to_rec(1, 42);
  if (menu_pos == 28) // Running Lights
    send_data_to_rec(1, 39);
  if (menu_pos == 29) // Weird Fire
    send_data_to_rec(1, 35);
  if (menu_pos == 30) // Strobing Moon
    send_data_to_rec(1, 44);
  if (menu_pos == 31) // Shining Moon
    send_data_to_rec(1, 1);
  if (menu_pos == 32) // OFF
    send_data_to_rec(0, esp_random() % 256);
  if (menu_pos == 33){ // Set Brightness
    send_data_to_rec(10, brightness); 
  }
    delay(11);
  }
  if (packet_delivery_status == true) {
    erase_default_hand();
    attackHandSprite.pushSprite(ATTACK_HAND_X + OFFSET_CAT_BY_X, ATTACK_HAND_Y + OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
    delay(SHOW_STATUS_CAT_FOR);
    erase_attack_hand();
    defaultHandSprite.pushSprite(DEFAULT_HAND_X + OFFSET_CAT_BY_X, DEFAULT_HAND_Y + OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
    disp_menu();

  } else {
    draw_hit_cat();
    delay(SHOW_STATUS_CAT_FOR);
    erase_cat();
    draw_cat();
    defaultHandSprite.pushSprite(DEFAULT_HAND_X + OFFSET_CAT_BY_X, DEFAULT_HAND_Y + OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
    disp_menu();
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    packet_delivery_status = true;
  }
}

void send_data_to_rec(char a_type, char m_value){
  int tmp_s[16];
  for(int i = 0; i < 10; i++){
      tmp_s[i] = ver_sq[i];
  }
  
  for(int i = 10; i < 14; i++){
      tmp_s[i] = esp_random() % 256;
  }

  tmp_s[14] = a_type;
  tmp_s[15] = m_value;
  /*
   for (int i = 0; i < 16; i++){
     Serial.print(res[i]);
  }
  Serial.println();
  */
  uint8_t ct1[32], pt1[32], key[64];
  int plen, clen, b, j;
  serpent_key skey;
  serpent_blk ct2;
  uint32_t *p;
  
  for (b=0; b<1; b++) {
    hex2bin(key);
  
    // set key
    memset (&skey, 0, sizeof (skey));
    p=(uint32_t*)&skey.x[0][0];
    
    serpent_setkey (&skey, key);
    //Serial.printf ("\nkey=");
    /*
    for (j=0; j<sizeof(skey)/sizeof(serpent_subkey_t)*4; j++) {
      if ((j % 8)==0) putchar('\n');
      Serial.printf ("%08X ", p[j]);
    }
    */
    for(int i = 0; i < 16; i++){
        ct2.b[i] = tmp_s[i];
    }
    serpent_encrypt (ct2.b, &skey, SERPENT_ENCRYPT);
    /*
    for (int i=0; i<16; i++) {
      if(ct2.b[i]<16)
        Serial.print("0");
      Serial.print(ct2.b[i],HEX);
    }
    */
     for(int i = 0; i <16; i++){
      myData.encr_with_serp[i] = ct2.b[i];
     }
     esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
     delayMicroseconds(240);
  }
}

void clear_variables() {
  string_for_ciphertext = "";
  return;
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

int getNum(char ch) {
  int num = 0;
  if (ch >= '0' && ch <= '9') {
    num = ch - 0x30;
  } else {
    switch (ch) {
    case 'A':
    case 'a':
      num = 10;
      break;
    case 'B':
    case 'b':
      num = 11;
      break;
    case 'C':
    case 'c':
      num = 12;
      break;
    case 'D':
    case 'd':
      num = 13;
      break;
    case 'E':
    case 'e':
      num = 14;
      break;
    case 'F':
    case 'f':
      num = 15;
      break;
    default:
      num = 0;
    }
  }
  return num;
}

char getChar(int num) {
  char ch;
  if (num >= 0 && num <= 9) {
    ch = char(num + 48);
  } else {
    switch (num) {
    case 10:
      ch = 'a';
      break;
    case 11:
      ch = 'b';
      break;
    case 12:
      ch = 'c';
      break;
    case 13:
      ch = 'd';
      break;
    case 14:
      ch = 'e';
      break;
    case 15:
      ch = 'f';
      break;
    }
  }
  return ch;
}

// Serpent (Below)

void split_by_eight_for_serp_only(char plntxt[], int k, int str_len) {
  char res[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };
  for (int i = 0; i < 8; i++) {
    if (i + k > str_len - 1)
      break;
    res[i] = plntxt[i + k];
  }
  for (int i = 8; i < 16; i++) {
    res[i] = esp_random() % 256;
  }
  int tmp_s[16];
  for (int i = 0; i < 16; i++) {
    tmp_s[i] = res[i];
  }

  uint8_t ct1[32], pt1[32], key[64];
  int plen, clen, b, j;
  serpent_key skey;
  serpent_blk ct2;
  uint32_t * p;

  for (b = 0; b < 1; b++) {
    hex2bin(key);

    // set key
    memset( & skey, 0, sizeof(skey));
    p = (uint32_t * ) & skey.x[0][0];

    serpent_setkey( & skey, key);

    for (int i = 0; i < 16; i++) {
      ct2.b[i] = tmp_s[i];
    }
    serpent_encrypt(ct2.b, & skey, SERPENT_ENCRYPT);
    /*
    for (int i = 0; i < 16; i++) {
      if (ct2.b[i] < 16)
        Serial.print("0");
      Serial.print(ct2.b[i], HEX);
    }
    */
    for (int i = 0; i < 16; i++) {
      if (ct2.b[i] < 16)
        string_for_ciphertext += "0";
      string_for_ciphertext += String(ct2.b[i], HEX);
    }
  }
}

void split_for_dec_serp_only(char ct[], int ct_len, int p) {
  int br = false;
  byte res[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };
  for (int i = 0; i < 32; i += 2) {
    if (i + p > ct_len - 1) {
      br = true;
      break;
    }
    if (i == 0) {
      if (ct[i + p] != 0 && ct[i + p + 1] != 0)
        res[i] = 16 * getNum(ct[i + p]) + getNum(ct[i + p + 1]);
      if (ct[i + p] != 0 && ct[i + p + 1] == 0)
        res[i] = 16 * getNum(ct[i + p]);
      if (ct[i + p] == 0 && ct[i + p + 1] != 0)
        res[i] = getNum(ct[i + p + 1]);
      if (ct[i + p] == 0 && ct[i + p + 1] == 0)
        res[i] = 0;
    } else {
      if (ct[i + p] != 0 && ct[i + p + 1] != 0)
        res[i / 2] = 16 * getNum(ct[i + p]) + getNum(ct[i + p + 1]);
      if (ct[i + p] != 0 && ct[i + p + 1] == 0)
        res[i / 2] = 16 * getNum(ct[i + p]);
      if (ct[i + p] == 0 && ct[i + p + 1] != 0)
        res[i / 2] = getNum(ct[i + p + 1]);
      if (ct[i + p] == 0 && ct[i + p + 1] == 0)
        res[i / 2] = 0;
    }
  }
  if (br == false) {
    uint8_t ct1[32], pt1[32], key[64];
    int plen, clen, i, j;
    serpent_key skey;
    serpent_blk ct2;
    uint32_t * p;

    for (i = 0; i < 1; i++) {
      hex2bin(key);

      // set key
      memset( & skey, 0, sizeof(skey));
      p = (uint32_t * ) & skey.x[0][0];

      serpent_setkey( & skey, key);

      for (int i = 0; i < 16; i++)
        ct2.b[i] = res[i];
      /*
      Serial.printf ("\n\n");
      for(int i = 0; i<16; i++){
      Serial.printf("%x", ct2.b[i]);
      Serial.printf(" ");
      */
    }
    //Serial.printf("\n");
    serpent_encrypt(ct2.b, & skey, SERPENT_DECRYPT);
      for (i = 0; i < 8; ++i) {
        string_for_ciphertext += (char(ct2.b[i]));
      }
  }
}

void encrypt_with_serpent_only(String input) {
  clear_variables();
  int str_len = input.length() + 1;
  char input_arr[str_len];
  input.toCharArray(input_arr, str_len);
  int p = 0;
  while (str_len > p + 1) {
    split_by_eight_for_serp_only(input_arr, p, str_len);
    p += 8;
  }
}

void decrypt_with_serpent_only(String ct) {
  clear_variables();
  int ct_len = ct.length() + 1;
  char ct_array[ct_len];
  ct.toCharArray(ct_array, ct_len);
  int ext = 0;
  while (ct_len > ext) {
    split_for_dec_serp_only(ct_array, ct_len, 0 + ext);
    ext += 32;
  }
}

// Serpent (Above)

void disp_centered_text(String t_disp, int y){
   tft.drawCentreString(t_disp, 160, y, 1);
}

void display_centered_highlighted_text(String text, int h, uint16_t hghl_color, uint16_t text_color) {
  tft.setTextColor(hghl_color);
  tft.drawCentreString(text, 160, h - 1, 1);
  tft.drawCentreString(text, 160, h + 1, 1);
  tft.drawCentreString(text, 159, h, 1);
  tft.drawCentreString(text, 161, h, 1);
  tft.setTextColor(text_color);
  tft.drawCentreString(text, 160, h, 1);
}

void disp_menu() {
    static int prev_start_item = -1;
    static int prev_menu_pos = -1;

    tft.setTextSize(MENU_TEXT_SIZE);
    int start_item = (menu_pos / MENU_ITEMS_PER_PAGE) * MENU_ITEMS_PER_PAGE;
    int end_item = min(start_item + MENU_ITEMS_PER_PAGE, total_menu_items);

    // If we've moved to a new page
    if (start_item != prev_start_item) {
        // Erase previous page
        for (int i = prev_start_item; i < prev_start_item + MENU_ITEMS_PER_PAGE && i < total_menu_items; i++) {
            int y_pos = (i - prev_start_item) * MENU_ITEM_SPACING + menu_shift_down;
            display_centered_highlighted_hebrew_text(menu_items[i], y_pos, COMMON_FILL_COLOR, COMMON_FILL_COLOR);
        }

        // Draw new page
        draw_cat();
        defaultHandSprite.pushSprite(DEFAULT_HAND_X + OFFSET_CAT_BY_X, DEFAULT_HAND_Y + OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
        for (int i = start_item; i < end_item; i++) {
            int y_pos = (i - start_item) * MENU_ITEM_SPACING + menu_shift_down;
            uint16_t text_color = (i == menu_pos) ? DEFAULT_TEXT_COLOR : COMMON_FILL_COLOR;
            display_centered_highlighted_hebrew_text(menu_items[i], y_pos, DEFAULT_HIGHLIGHT_COLOR, text_color);
        }
    } else {
        // We're on the same page, just update changed items
        if (prev_menu_pos != -1) {
            // Update previous active item
            int prev_y_pos = (prev_menu_pos - start_item) * MENU_ITEM_SPACING + menu_shift_down;
            display_centered_highlighted_hebrew_text(menu_items[prev_menu_pos], prev_y_pos, DEFAULT_HIGHLIGHT_COLOR, COMMON_FILL_COLOR);
        }

        // Update new active item
        int new_y_pos = (menu_pos - start_item) * MENU_ITEM_SPACING + menu_shift_down;
        display_centered_highlighted_hebrew_text(menu_items[menu_pos], new_y_pos, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);
    }

    prev_start_item = start_item;
    prev_menu_pos = menu_pos;
}

void check_bounds_and_disp_menu() {
    if (menu_pos >= total_menu_items)
        menu_pos = 0;
    if (menu_pos < 0)
        menu_pos = total_menu_items - 1;
    disp_menu();
}

void load_image_into_sprite(fs::FS &fs, const char *path, TFT_eSprite* sprite = nullptr)
{
  currentSprite = sprite;
  sx = png_dx;
  sy = png_dy;
  pc = 0;

  fs::File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  pngle_t *pngle = pngle_new();
  pngle_set_draw_callback(pngle, pngle_on_draw);

  uint8_t buf[1024];
  int remain = 0;
  int len;

  if (!currentSprite) tft.startWrite();

  while ((len = file.read(buf + remain, sizeof(buf) - remain)) > 0) {
    int fed = pngle_feed(pngle, buf, remain + len);
    if (fed < 0) {
      Serial.printf("ERROR: %s\n", pngle_error(pngle));
      break;
    }

    remain = remain + len - fed;
    if (remain > 0) memmove(buf, buf + fed, remain);
  }

  if (!currentSprite) tft.endWrite();

  pngle_destroy(pngle);
  file.close();
}

void erase_default_hand() {
  tft.fillRect(58 + OFFSET_CAT_BY_X, 14 + OFFSET_CAT_BY_Y, 19, 13, COMMON_FILL_COLOR);
  tft.fillRect(61 + OFFSET_CAT_BY_X, 27 + OFFSET_CAT_BY_Y, 10, 37, COMMON_FILL_COLOR);
  tft.fillRect(55 + OFFSET_CAT_BY_X, 41 + OFFSET_CAT_BY_Y, 6, 30, COMMON_FILL_COLOR);
  tft.fillRect(48 + OFFSET_CAT_BY_X, 50 + OFFSET_CAT_BY_Y, 7, 14, COMMON_FILL_COLOR);
}

void erase_attack_hand() {
  tft.fillRect(48 + OFFSET_CAT_BY_X, 50 + OFFSET_CAT_BY_Y, 27, 13, COMMON_FILL_COLOR);
  tft.fillRect(59 + OFFSET_CAT_BY_X, 63 + OFFSET_CAT_BY_Y, 7, 8, COMMON_FILL_COLOR);
  tft.fillRect(61 + OFFSET_CAT_BY_X, 18 + OFFSET_CAT_BY_Y, 25, 32, COMMON_FILL_COLOR);
}

void erase_cat(){
  tft.fillRect(OFFSET_CAT_BY_X, OFFSET_CAT_BY_Y, 59, 99, COMMON_FILL_COLOR);
  erase_default_hand();
  erase_attack_hand();
}

void draw_cat() {
  catSprite.pushSprite(OFFSET_CAT_BY_X, OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
}

void draw_hit_cat() {
  hitCatSprite.pushSprite(OFFSET_CAT_BY_X, OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
}

void draw_sunflower() {
  sunflowerSprite.createSprite(65, 120);
  sunflowerSprite.setColorDepth(16);
  sunflowerSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/Sunflower_65x120.png", &sunflowerSprite);
  sunflowerSprite.pushSprite(SUNFLOWER_X, SUNFLOWER_Y, TFT_TRANSPARENT);
}

void draw_soil(){
  for (int i = 0; i < 320; i++){
    for (int j = 0; j < 29; j++){
      tft.drawPixel(i, j + 211, Soil_320_29[i][j]);
    }
  }
}

void load_cat_sprites(){
  delay(24);
  // Create and load default hand sprite with transparent background
  defaultHandSprite.createSprite(DEFAULT_HAND_WIDTH, DEFAULT_HAND_HEIGHT);
  defaultHandSprite.setColorDepth(16);
  defaultHandSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/Default_hand_26_57.png", &defaultHandSprite);
  delay(24);
  // Create and load attack hand sprite with transparent background
  attackHandSprite.createSprite(ATTACK_HAND_WIDTH, ATTACK_HAND_HEIGHT);
  attackHandSprite.setColorDepth(16);
  attackHandSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/Attack_hand_38_53.png", &attackHandSprite);
  delay(24);
  catSprite.createSprite(59, 99);
  catSprite.setColorDepth(16);
  catSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/One_hand_cat_59_99.png", &catSprite);
  delay(24);
  hitCatSprite.createSprite(74, 99);
  hitCatSprite.setColorDepth(16);
  hitCatSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/Hit_cat_74_99.png", &hitCatSprite);
}

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COMMON_FILL_COLOR);
  tft.setTextSize(2);
  display_centered_highlighted_text("Addressable", 10, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);
  display_centered_highlighted_text("RGB LED Strip", 30, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);
  display_centered_highlighted_text("Controller", 50, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);
  Serial.begin(115200);
  draw_soil();
  tft.setTextSize(1);
  display_centered_highlighted_text("github.com/Northstrix/Lantern", 230, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);
  tft.setTextSize(2);
  Wire.begin(NUNCHUCK_SDA, NUNCHUCK_SCL);
  wii_nunchuk.begin();
  bool colorSwap = false;

  while (!wii_nunchuk.connect()) {
    colorSwap = !colorSwap;
    if (colorSwap) {
      display_centered_highlighted_text("Connect Nunchuk", 180, DEFAULT_TEXT_COLOR, DEFAULT_RED_COLOR);
    } else {
      display_centered_highlighted_text("Connect Nunchuk", 180, DEFAULT_RED_COLOR, DEFAULT_TEXT_COLOR);
    }
    delay(1000);
  }
  tft.fillRect(60, 178, 200, 22, COMMON_FILL_COLOR);
  load_cat_sprites();
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer( & peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  load_cat_sprites();
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  load_cat_sprites();
  bool break_start_lp = false;
  new_time_millis = 0;

  while (break_start_lp == false) {
    unsigned long currentTime = millis();

    // Check if 1 second has passed
    if (currentTime - new_time_millis >= 1000) {
      colorSwap = !colorSwap;
      new_time_millis = currentTime;

      if (colorSwap) {
        display_centered_highlighted_hebrew_text("LicX hTcL", 88, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);
      } else {
        display_centered_highlighted_hebrew_text("LicX hTcL", 88, DEFAULT_TEXT_COLOR, DEFAULT_HIGHLIGHT_COLOR);
      }
    }

    byte input_from_n = get_nunchuk_input();
    if (input_from_n == 13 || input_from_n == 27) {
      break_start_lp = true;
      break;
    }
  }
  tft.fillRect(20, 8, 280, 192, COMMON_FILL_COLOR);
  display_centered_highlighted_hebrew_text("LicX hTcL", 88, COMMON_FILL_COLOR, COMMON_FILL_COLOR);
  draw_cat();
  defaultHandSprite.pushSprite(DEFAULT_HAND_X + OFFSET_CAT_BY_X, DEFAULT_HAND_Y + OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
  disp_menu();
  draw_sunflower();
}

int prev_menu_pos = 0;

void loop() {
  byte input_data = get_nunchuk_input();
  if (input_data != 0) {
    if (input_data == 131 || input_data == 132) { // Up or Down
      menu_pos += (input_data == 131) ? -1 : 1;
      if (prev_menu_pos == 33 && menu_pos != 33) {
          tft.fillRect(120, 106, 80, 40, COMMON_FILL_COLOR);
      }
      check_bounds_and_disp_menu();
      
      prev_menu_pos = menu_pos; // Update previous menu position
    } else if (input_data == 13 || input_data == 27) { // Enter
      send_smt();
    }

    Serial.println(menu_pos);
    if (menu_pos == 33) {
      tft.setTextSize(4);
      display_centered_highlighted_text(String(brightness), 110, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);

      // Handle brightness adjustment
      if (input_data == 130) { // Right
        // Erase old value
        display_centered_highlighted_text(String(brightness), 110, COMMON_FILL_COLOR, COMMON_FILL_COLOR);
        
        brightness++;
        if (brightness > 255) brightness = 0;
        
        // Display new value
        display_centered_highlighted_text(String(brightness), 110, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);
      } 
      else if (input_data == 129) { // Left
        // Erase old value
        display_centered_highlighted_text(String(brightness), 110, COMMON_FILL_COLOR, COMMON_FILL_COLOR);
        
        brightness--;
        if (brightness < 0) brightness = 255;
        
        // Display new value
        display_centered_highlighted_text(String(brightness), 110, DEFAULT_HIGHLIGHT_COLOR, DEFAULT_TEXT_COLOR);
      }

      // Reset text size to default
      tft.setTextSize(2);
    }

    input_data = 0;
  }
}

// Functions for custom Hebrew font below

#define letter_spacing_pxls 6
#define space_between_letter 16
#define regular_shift_down 16
#define shift_down_for_mem 12
#define shift_down_for_shin 13
#define shift_down_for_tsadi 8
#define shift_down_for_dot_and_comma 38

void display_centered_highlighted_hebrew_text(String text_to_print, int y, uint16_t highlight_color, uint16_t font_color){
  print_custom_hebrew_font(text_to_print, y + 1, get_offset(text_to_print), highlight_color);
  print_custom_hebrew_font(text_to_print, y - 1, get_offset(text_to_print), highlight_color);
  print_custom_hebrew_font(text_to_print, y, get_offset(text_to_print) + 1, highlight_color);
  print_custom_hebrew_font(text_to_print, y, get_offset(text_to_print) - 1, highlight_color);
  print_custom_hebrew_font(text_to_print, y, get_offset(text_to_print), font_color);
}

int get_offset(String text_to_print) {
  int shift_right = 320;

  for (int s = 0; s < text_to_print.length(); s++) {
    if (text_to_print.charAt(s) == ' ') { // Space
      shift_right -= space_between_letter;
    } else if (text_to_print.charAt(s) == 'A') { // Alef
      shift_right -= sizeof(Alef) / sizeof(Alef[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == '"') { // Apostrophe
      shift_right -= sizeof(Apostrophe) / sizeof(Apostrophe[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'a') { // Ayin
      shift_right -= sizeof(Ayin) / sizeof(Ayin[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'b') { // Bet
      shift_right -= sizeof(Bet) / sizeof(Bet[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'c') { // Chet
      shift_right -= sizeof(Chet) / sizeof(Chet[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'd') { // Dalet
      shift_right -= sizeof(Dalet) / sizeof(Dalet[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'm') { // ending mem
      shift_right -= sizeof(ending_mem) / sizeof(ending_mem[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'n') { // ending nun
      shift_right -= sizeof(ending_nun) / sizeof(ending_nun[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'f') { // ending pe
      shift_right -= sizeof(ending_pe) / sizeof(ending_pe[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'q') { // ending qaf
      shift_right -= sizeof(ending_qaf) / sizeof(ending_qaf[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'x') { // ending tsadi
      shift_right -= sizeof(ending_tsadi) / sizeof(ending_tsadi[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'g') { // Gimel
      shift_right -= sizeof(Gimel) / sizeof(Gimel[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'h') { // He
      shift_right -= sizeof(He) / sizeof(He[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'L') { // Lamed
      shift_right -= sizeof(Lamed) / sizeof(Lamed[0]);
      if (s != 0)
        shift_right += 12;
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'M') { // Mem
      shift_right -= sizeof(Mem) / sizeof(Mem[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'N') { // Nun
      shift_right -= sizeof(Nun) / sizeof(Nun[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'p') { // Pe
      shift_right -= sizeof(Pe) / sizeof(Pe[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'C') { // Qaf
      shift_right -= sizeof(Qaf) / sizeof(Qaf[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'k') { // Qof
      shift_right -= sizeof(Qof) / sizeof(Qof[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'r') { // Resh
      shift_right -= sizeof(Resh) / sizeof(Resh[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 's') { // Samekh
      shift_right -= sizeof(Samekh) / sizeof(Samekh[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'S') { // Shin
      shift_right -= sizeof(Shin) / sizeof(Shin[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'T') { // Tav
      shift_right -= sizeof(Tav) / sizeof(Tav[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 't') { // Tet
      shift_right -= sizeof(Tet) / sizeof(Tet[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'X') { // Tsadi
      shift_right -= sizeof(Tsadi) / sizeof(Tsadi[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'i') { // Vav
      shift_right -= sizeof(Vav) / sizeof(Vav[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == '\'') { // Yod
      shift_right -= sizeof(Yod) / sizeof(Yod[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == 'z') { // Zayin
      shift_right -= sizeof(Zayin) / sizeof(Zayin[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == '.') { // Dot
      shift_right -= sizeof(dot) / sizeof(dot[0]);
      shift_right -= letter_spacing_pxls;
    } else if (text_to_print.charAt(s) == ',') { // Comma
      shift_right -= sizeof(comma) / sizeof(comma[0]);
      shift_right -= letter_spacing_pxls;
    }
  }
  shift_right += letter_spacing_pxls;
  return shift_right / 2;
}

void print_custom_hebrew_font(String text_to_print, int y, int offset_from_the_right, uint16_t font_color){
  int shift_right = 320 - offset_from_the_right;
  for (int s = 0; s < text_to_print.length(); s++){ // Traverse the string
    
    if (text_to_print.charAt(s) == ' '){ // Space
      shift_right -= space_between_letter;
    }
    
    if (text_to_print.charAt(s) == 'A'){ // Alef
      shift_right -= sizeof(Alef)/sizeof(Alef[0]);
      for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 24; j++) {
          if (Alef[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '"'){ // Apostrophe
      shift_right -= sizeof(Apostrophe)/sizeof(Apostrophe[0]);
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 10; j++) {
          if (Apostrophe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'a'){ // Ayin
      shift_right -= sizeof(Ayin)/sizeof(Ayin[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 24; j++) {
          if (Ayin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'b'){ // Bet
      shift_right -= sizeof(Bet)/sizeof(Bet[0]);
      for (int i = 0; i < 22; i++) {
        for (int j = 0; j < 24; j++) {
          if (Bet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'c'){ // Chet
      shift_right -= sizeof(Chet)/sizeof(Chet[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 24; j++) {
          if (Chet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'd'){ // Dalet
      shift_right -= sizeof(Dalet)/sizeof(Dalet[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Dalet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'm'){ // ending mem
      shift_right -= sizeof(ending_mem)/sizeof(ending_mem[0]);
      for (int i = 0; i < 23; i++) {
        for (int j = 0; j < 24; j++) {
          if (ending_mem[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'n'){ // ending nun
      shift_right -= sizeof(ending_nun)/sizeof(ending_nun[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 39; j++) {
          if (ending_nun[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'f'){ // ending pe
      shift_right -= sizeof(ending_pe)/sizeof(ending_pe[0]);
      for (int i = 0; i < 23; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_pe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'q'){ // ending qaf
      shift_right -= sizeof(ending_qaf)/sizeof(ending_qaf[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_qaf[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'x'){ // ending tsadi
      shift_right -= sizeof(ending_tsadi)/sizeof(ending_tsadi[0]);
      for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_tsadi[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'g'){ // Gimel
      shift_right -= sizeof(Gimel)/sizeof(Gimel[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 24; j++) {
          if (Gimel[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'h'){ // He
      shift_right -= sizeof(He)/sizeof(He[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 24; j++) {
          if (He[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'L'){ // Lamed
      shift_right -= sizeof(Lamed)/sizeof(Lamed[0]);
      if (s != 0)
        shift_right += 12;
      for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 40; j++) {
          if (Lamed[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'M'){ // Mem
      shift_right -= sizeof(Mem)/sizeof(Mem[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 29; j++) {
          if (Mem[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_mem, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'N'){ // Nun
      shift_right -= sizeof(Nun)/sizeof(Nun[0]);
      for (int i = 0; i < 14; i++) {
        for (int j = 0; j < 24; j++) {
          if (Nun[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'p'){ // Pe
      shift_right -= sizeof(Pe)/sizeof(Pe[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Pe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'C'){ // Qaf
      shift_right -= sizeof(Qaf)/sizeof(Qaf[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Qaf[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'k'){ // Qof
      shift_right -= sizeof(Qof)/sizeof(Qof[0]);
      for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 38; j++) {
          if (Qof[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'r'){ // Resh
      shift_right -= sizeof(Resh)/sizeof(Resh[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Resh[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 's'){ // Samekh
      shift_right -= sizeof(Samekh)/sizeof(Samekh[0]);
      for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 24; j++) {
          if (Samekh[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'S'){ // Shin
      shift_right -= sizeof(Shin)/sizeof(Shin[0]);
      for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 27; j++) {
          if (Shin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_shin, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'T'){ // Tev
      shift_right -= sizeof(Tav)/sizeof(Tav[0]);
      for (int i = 0; i < 33; i++) {
        for (int j = 0; j < 24; j++) {
          if (Tav[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 't'){ // Tet
      shift_right -= sizeof(Tet)/sizeof(Tet[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Tet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'X'){ // Tsadi
      shift_right -= sizeof(Tsadi)/sizeof(Tsadi[0]);
      for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 32; j++) {
          if (Tsadi[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_tsadi, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'i'){ // Vav
      shift_right -= sizeof(Vav)/sizeof(Vav[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 24; j++) {
          if (Vav[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '\''){ // Yod
      shift_right -= sizeof(Yod)/sizeof(Yod[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 10; j++) {
          if (Yod[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'z'){ // Zayin
      shift_right -= sizeof(Zayin)/sizeof(Zayin[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Zayin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '.'){ // Dot
      shift_right -= sizeof(dot)/sizeof(dot[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
          if (dot[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_dot_and_comma, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == ','){ // Comma
      shift_right -= sizeof(comma)/sizeof(comma[0]);
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 7; j++) {
          if (comma[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_dot_and_comma, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '!'){ // Exclamation mark
      shift_right -= sizeof(excl)/sizeof(excl[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 24; j++) {
          if (excl[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '?'){ // Question mark
      shift_right -= sizeof(question_mark)/sizeof(question_mark[0]);
      for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 24; j++) {
          if (question_mark[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }
  }
}
