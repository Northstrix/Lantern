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
// !!! Before uploading this sketch -
// Switch the partition scheme to the
// "No OTA (2MB APP/2MB SPIFFS)" !!!
#include <SPI.h>
#include <FS.h>
#include "SPIFFS.h"
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Lock_screens.h"
#include "serpent.h"
#define TFT_CS1         5
#define TFT_RST1        19
#define TFT_DC1         22
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS1, TFT_DC1, TFT_RST1);

#include <Keypad.h>
#define ROW_NUM     4
#define COLUMN_NUM  4

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

uint8_t broadcastAddress[] = {0x5C, 0xCF, 0x7F, 0xFD, 0x85, 0x1D}; // Receiver's MAC address

char p_k[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

uint16_t c;
String keyboard_input;
String dec_st;
int curr_key;
int menu_pos;
bool finish_input;
bool decrypt_tag;
bool act;
const uint16_t current_inact_clr = 0x051b;
const uint16_t five_six_five_red_color = 0xf940;

byte pin_rows[ROW_NUM]      = {13, 12, 14, 27};
byte pin_column[COLUMN_NUM] = {26, 25, 33, 32};
Keypad keypad = Keypad( makeKeymap(p_k), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

typedef struct struct_message {
  char encr_with_serp[16];
} struct_message;

struct_message myData;

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
  keyboard_input = "";
  dec_st = "";
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
        dec_st += "0";
      dec_st += String(ct2.b[i], HEX);
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
        dec_st += (char(ct2.b[i]));
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
   int16_t x1, y1;
   uint16_t w, h;
   tft.getTextBounds(t_disp, 160, 0, &x1, &y1, &w, &h);
   tft.setCursor(80 - (w / 2), y);
   tft.print(t_disp);
}

void set_stuff_for_input(String blue_inscr) {
  curr_key = 65;
  tft.fillScreen(0x0000);
  tft.setTextSize(2);
  tft.setTextColor(0xffff);
  tft.setCursor(2, 0);
  tft.print("Char'");
  tft.setCursor(74, 0);
  tft.print("'");
  disp();
  tft.setCursor(0, 20);
  tft.setTextSize(2);
  tft.setTextColor(current_inact_clr);
  tft.print(blue_inscr);
}

void change_char() {
  if (keyboard_input.length() > 0)
    curr_key = keyboard_input.charAt(keyboard_input.length() - 1);
}

void disp() {
  //gfx->fillScreen(0x0000);
  tft.setTextSize(2);
  tft.setTextColor(0xffff);
  tft.fillRect(62, 0, 10, 16, 0x0000);
  tft.setCursor(62, 0);
  tft.print(char(curr_key));
  tft.setTextColor(0xffff);
  tft.setCursor(0, 40);
  tft.print(keyboard_input);
}

void disp_stars() {
  //gfx->fillScreen(0x0000);
  tft.setTextSize(2);
  tft.setTextColor(0xffff);
  tft.fillRect(62, 0, 10, 16, 0x0000);
  tft.setCursor(62, 0);
  tft.print(char(curr_key));
  tft.setTextColor(0xffff);
  tft.setTextSize(2);
  int plnt = keyboard_input.length();
  String stars = "";
  for (int i = 0; i < plnt; i++) {
    stars += "*";
  }
  tft.setTextColor(0xffff);
  tft.setCursor(0, 40);
  tft.print(stars);
}

void encdr_and_keyb_input() {
  finish_input = false;
  while (finish_input == false) {

   char key = keypad.getKey();
    if (key) {
      
    if (key == '*') {
      if (keyboard_input.length() > 0)
        keyboard_input.remove(keyboard_input.length() - 1, 1);
      //Serial.println(keyboard_input);
      tft.fillRect(0, 40, 160, 128, 0x0000);
      //Serial.println(keyboard_input);
      change_char();
      disp();
    }

    else if (key == 'C'){
      act = false;
      finish_input = true;
    }

    else if (key == '#'){
      finish_input = true;
    }

    else{
      keyboard_input += char(key);
        change_char();
        disp();
    }
    
   }
    delayMicroseconds(400);
  }
}

void star_encdr_and_keyb_input() {
  finish_input = false;
  while (finish_input == false) {

   char key = keypad.getKey();
    if (key) {
      
    if (key == '*') {
      if (keyboard_input.length() > 0)
        keyboard_input.remove(keyboard_input.length() - 1, 1);
      //Serial.println(keyboard_input);
      tft.fillRect(0, 40, 160, 128, 0x0000);
      //Serial.println(keyboard_input);
      change_char();
      disp_stars();
    }

    else if (key == '#'){
      finish_input = true;
    }

    else{
      keyboard_input += char(key);
        change_char();
        disp_stars();
    }
    
   }
    delayMicroseconds(400);
  }
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

void write_to_file_with_overwrite(fs::FS &fs, String filename, String content) {
   //Serial.printf("Writing file: %s\r\n", filename);

   File file = fs.open(filename, FILE_WRITE);
   if(!file){
      //Serial.println("− failed to open file for writing");
      return;
   }
   if(file.print(content)){
      //Serial.println("− file written");
   }else {
      //Serial.println("− frite failed");
   }
}

String read_file(fs::FS &fs, String filename) {
  String file_content;
   //Serial.printf("Reading file: %s\r\n", filename);

   File file = fs.open(filename);
   if(!file || file.isDirectory()){
       //Serial.println("− failed to open file for reading");
       return "-1";
   }

   //Serial.println("− read from file:");
   while(file.available()){
      file_content += char(file.read());
   }
   return file_content;
}

void continue_to_unlock() {
  if (read_file(SPIFFS, "/lpass").equals("-1"))
    set_pass();
  else
    unlock_lantern();
  return;
}

void set_pass() {
  clear_variables();
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  set_stuff_for_input("Set Password:");
  encdr_and_keyb_input();
  tft.setTextSize(1);
  tft.fillScreen(0x0000);
  disp_centered_text("Setting Password", 15);
  //Serial.println(keyboard_input);
  dec_st = "";
  encrypt_with_serpent_only(keyboard_input);
  //Serial.println(dec_st);
  write_to_file_with_overwrite(SPIFFS, "/lpass", dec_st);
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  disp_centered_text("Password set", 15);
  disp_centered_text("successfully", 35);
  delay(1000);
  menu_pos = 0;
  disp_menu();
}

void unlock_lantern() {
  clear_variables();
  tft.fillScreen(0x0000);
  tft.setTextColor(0xffff);
  tft.setTextSize(2);
  set_stuff_for_input("Enter passwd:");
  star_encdr_and_keyb_input();
  String pssbck = keyboard_input;
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  disp_centered_text("Unlocking Lantern", 15);
  disp_centered_text("Please wait", 35);
  disp_centered_text("for a while", 55);
  decrypt_with_serpent_only(read_file(SPIFFS, "/lpass"));
  String dec_psswd;
  for (int i = 0; i < dec_st.length(); i++){
    if (dec_st.charAt(i) > 31 && dec_st.charAt(i) < 127)
      dec_psswd += dec_st.charAt(i);
  }
  //Serial.println(pssbck);
  //Serial.println(dec_psswd);
  bool next_act = pssbck.equals(dec_psswd);
  clear_variables();
  tft.fillScreen(0x0000);
  if (next_act == true) {
    tft.setTextSize(1);
    disp_centered_text("Lantern unlocked", 15);
    disp_centered_text("successfully", 35);
    delay(1000);
    menu_pos = 0;
    disp_menu();
    return;
  } else {
    tft.setTextSize(1);
    tft.setTextColor(five_six_five_red_color);
    disp_centered_text("Wrong Password!", 15);
    tft.setTextColor(0xffff);
    disp_centered_text("Please reboot", 55);
    disp_centered_text("the device", 75);
    disp_centered_text("and try again", 95);
    for (;;)
      delay(1000);
  }
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
    disp_centered_text("White To Red", 39);
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
    disp_centered_text("White To Red", 39);
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
    disp_centered_text("White To Red", 39);
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
    disp_centered_text("White To Red", 39);
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
    disp_centered_text("White To Red", 39);
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
    disp_centered_text("White To Red", 39);
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
    disp_centered_text("White To Red", 39);
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
    disp_centered_text("White To Red", 39);
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
    disp_centered_text("OFF", 9);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("Set Brightness", 24);
  }
  if (menu_pos == 33){
    tft.fillScreen(0x0000);
    tft.setTextColor(current_inact_clr);
    disp_centered_text("OFF", 9);
    tft.setTextColor(0xffff);
    disp_centered_text("Set Brightness", 24);
  }
}

void send_smt(){
  if (menu_pos == 0) // Rainbow Fade
    send_data_to_rec(1, 2);
  if (menu_pos == 1) // Rainbow Loop
    send_data_to_rec(1, 3);
  if (menu_pos == 2) // Rainbow Burst
    send_data_to_rec(1, 4);
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
  if (menu_pos == 12) // Weird Red
    send_data_to_rec(1, 13);
  if (menu_pos == 13) // Color March
    send_data_to_rec(1, 14);
  if (menu_pos == 14) // American Carnival
    send_data_to_rec(1, 15);
  if (menu_pos == 15) // Breathing Green
    send_data_to_rec(1, 16);
  if (menu_pos == 16) // Running Red
    send_data_to_rec(1, 20);
  if (menu_pos == 17) // Center Blue
    send_data_to_rec(1, 21);
  if (menu_pos == 18) // Fire
    send_data_to_rec(1, 22);
  if (menu_pos == 19) // Another Rainbow
    send_data_to_rec(1, 23);
  if (menu_pos == 20) // Shooting Stars
    send_data_to_rec(1, 25);
  if (menu_pos == 21) // Emergency Strobe
    send_data_to_rec(1, 26);
  if (menu_pos == 22) // Circular Run
    send_data_to_rec(1, 27);
  if (menu_pos == 23) // Jumping Red
    send_data_to_rec(1, 28);
  if (menu_pos == 24) // Ascending Green
    send_data_to_rec(1, 29);
  if (menu_pos == 25) // Green Ring
    send_data_to_rec(1, 33);
  if (menu_pos == 26) // Fast Red
    send_data_to_rec(1, 34);
  if (menu_pos == 27) // Weird Fire
    send_data_to_rec(1, 35);
  if (menu_pos == 28) // Running Lights
    send_data_to_rec(1, 39);
  if (menu_pos == 29) // Theater Chase
    send_data_to_rec(1, 42);
  if (menu_pos == 30) // Strobing Moon
    send_data_to_rec(1, 44);
  if (menu_pos == 31) // Shining Moon
    send_data_to_rec(1, 1);
  if (menu_pos == 32) // OFF
    send_data_to_rec(0, esp_random() % 256);
  if (menu_pos == 33){ // Set Brightness
    act = true;
    clear_variables();
    set_stuff_for_input("Set Brightnss");
    encdr_and_keyb_input();
    if (act == true) {
     send_data_to_rec(10, keyboard_input.toInt()); 
    }
    disp_menu();
  }
}

void check_bounds_and_disp_menu(){
      if (menu_pos > 33)
        menu_pos = 0;

      if (menu_pos < 0)
        menu_pos = 33;
        
      disp_menu();
}

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // Add peer        
  if (esp_now_add_peer(&peerInfo) == ESP_OK){

  }
  else{
    Serial.println("Failed to add peer");
    return;
  }
  
  draw_lock_screen(esp_random() % 14);
  disp_centered_text("Press Any Key", 115);
  finish_input = false;
  while (finish_input == false) {
   char key = keypad.getKey();
    if (key){
      finish_input = true;
      break;
    }
    delayMicroseconds(400);
  }
  if (SPIFFS.begin(true)) {} else {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
    continue_to_unlock();
}

void loop() {
   char key = keypad.getKey();
    if (key) {
      
      if (key == '8') {
        menu_pos--;
        check_bounds_and_disp_menu();
      }

      else if (key == '0'){
        menu_pos++;
        check_bounds_and_disp_menu();
      }

      else if (key == '#'){
        send_smt();
      }
    }
}
