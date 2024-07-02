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
*/

#include <SPI.h>
#include <FS.h>
#include "SPIFFS.h"
#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include "serpent.h"
#include <NintendoExtensionCtrl.h>

TFT_eSPI tft = TFT_eSPI();
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

esp_now_peer_info_t peerInfo;

bool pressed_c = false;
bool pressed_z = false;
bool held_left = false;
bool held_up = false;
bool held_right = false;
bool held_down = false;
bool c_functions_as_enter = true;
bool stick_up_to_add_char = true;
bool right_fast_scroll = false;
bool left_fast_scroll = false;
bool up_fast_scroll = false;
bool down_fast_scroll = false;
bool do_right_fast_scroll = false;
bool do_left_fast_scroll = false;
bool do_up_fast_scroll = false;
bool do_down_fast_scroll = false;

byte threshold = 16;
int wait_till_fast_scroll = 500;
int delay_for_fast_scroll = 50;

typedef struct struct_message {
  char encr_with_serp[16];
} struct_message;

struct_message myData;

void setup() {
  Serial.begin(115200);
  Wire.begin(NUNCHUCK_SDA, NUNCHUCK_SCL);
  wii_nunchuk.begin();
  while (!wii_nunchuk.connect()) {
    Serial.println("Nunchuk not detected!");
    delay(1000);
  }

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  disp_menu();
}

void loop() {
  byte input_data = get_nunchuk_input();
  if (input_data != 0) {
    if (input_data == 131 || input_data == 132) { // Up or Down
      menu_pos += (input_data == 131) ? -1 : 1;
      check_bounds_and_disp_menu();
    } else if (input_data == 13) { // Enter
      send_smt();
    }
  }
}

byte get_nunchuk_input() {
  boolean success = wii_nunchuk.update();  // Get new data from the controller

  if (!success) {
    Serial.println("Controller disconnected!");
    delay(1000);
    return 0;
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

  // Check joystick directions (including fast scroll)
  if (XAxis > (255 - threshold)) {
    return check_fast_scroll(held_right, right_fast_scroll, do_right_fast_scroll, 130, XAxis > (255 - threshold));
  } else if (XAxis < threshold) {
    return check_fast_scroll(held_left, left_fast_scroll, do_left_fast_scroll, 129, XAxis < threshold);
  } else if (YAxis > (255 - threshold)) {
    return check_fast_scroll(held_up, up_fast_scroll, do_up_fast_scroll, stick_up_to_add_char ? 131 : 132, YAxis > (255 - threshold));
  } else if (YAxis < threshold) {
    return check_fast_scroll(held_down, down_fast_scroll, do_down_fast_scroll, stick_up_to_add_char ? 132 : 131, YAxis < threshold);
  }

  return 0; // No input detected
}

byte check_fast_scroll(bool &held, bool &fast_scroll, bool &do_fast_scroll, byte return_value, bool condition) {
  if (held) {
    if (!fast_scroll) {
      do_fast_scroll = true;
      for (int i = 0; i < wait_till_fast_scroll; i++) {
        wii_nunchuk.update();
        delay(1);
        if (!condition) {
          do_fast_scroll = false;
          break;
        }
      }
      if (do_fast_scroll) fast_scroll = true;
    }
    if (fast_scroll) {
      delay(delay_for_fast_scroll);
      return return_value;
    }
  }
  if (!held) {
    held = true;
    return return_value;
  }
  return 0;
}

void send_smt() {
  if (menu_pos >= 0 && menu_pos <= 31) {
    send_data_to_rec(1, menu_pos + 1);
  } else if (menu_pos == 32) {
    send_data_to_rec(0, esp_random() % 256);
  } else if (menu_pos == 33) {
    act = true;
    clear_variables();
    set_stuff_for_input("Set Brightnss");
    Serial.println("Brightness");
    encdr_and_keyb_input();
    if (act == true) {
      send_data_to_rec(10, keyboard_input.toInt()); 
    }
    disp_menu();
  }
}

void check_bounds_and_disp_menu() {
  if (menu_pos > 33)
    menu_pos = 0;
  if (menu_pos < 0)
    menu_pos = 33;
  disp_menu();
}

void set_stuff_for_input(String input_string) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println(input_string);
}

void encdr_and_keyb_input() {
  tft.setCursor(0, 10);
  while (true) {
    byte input = get_nunchuk_input();
    if (input == 13) { // Enter
      break;
    } else if (input == 27) { // Cancel
      act = false;
      break;
    } else if (input >= 32 && input <= 126) { // Printable ASCII characters
      keyboard_input += (char)input;
      tft.print((char)input);
    }
  }
}


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
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
   tft.drawCentreString(t_disp, 160, y, 1);
}

void disp_centered_text_b_w(String text, int h, uint16_t hghl_color, uint16_t text_color) {
  tft.setTextColor(hghl_color);
  tft.drawCentreString(text, 160, h - 1, 1);
  tft.drawCentreString(text, 160, h + 1, 1);
  tft.drawCentreString(text, 159, h, 1);
  tft.drawCentreString(text, 161, h, 1);
  tft.setTextColor(text_color);
  tft.drawCentreString(text, 160, h, 1);
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
