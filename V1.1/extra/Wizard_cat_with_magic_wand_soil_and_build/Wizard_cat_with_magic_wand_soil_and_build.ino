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
#define USE_LINE_BUFFER  // Enable for faster rendering
#include <TFT_eSPI.h>              // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();         // Invoke custom library
TFT_eSprite catSprite = TFT_eSprite(&tft);
TFT_eSprite sunflowerSprite = TFT_eSprite(&tft);
TFT_eSprite defaultHandSprite = TFT_eSprite(&tft);
TFT_eSprite attackHandSprite = TFT_eSprite(&tft);

// Include SPIFFS
#define FS_NO_GLOBALS
#include <FS.h>
#include "SPIFFS.h"
#include <math.h>
#include "pngle.h"
#include "soil_sprite.h"

#define COMMON_FILL_COLOR 0x4d7a
#define OFFSET_CAT_BY_X 8
#define OFFSET_CAT_BY_Y 112

// Define constants for hand positions and sizes
const int DEFAULT_HAND_X = 48;
const int DEFAULT_HAND_Y = 14;
const int DEFAULT_HAND_WIDTH = 26;
const int DEFAULT_HAND_HEIGHT = 57;

const int ATTACK_HAND_X = 48;
const int ATTACK_HAND_Y = 18;
const int ATTACK_HAND_WIDTH = 38;
const int ATTACK_HAND_HEIGHT = 53;
#define SUNFLOWER_X 247
#define SUNFLOWER_Y 91

#define LINE_BUF_SIZE 240
uint16_t lbuf[LINE_BUF_SIZE];
int16_t sx = 0;
int16_t sy = 0;
uint32_t pc = 0;

int16_t png_dx = 0, png_dy = 0;

TFT_eSprite* currentSprite = nullptr;

void setPngPosition(int16_t x, int16_t y)
{
  png_dx = x;
  png_dy = y;
}

void pngle_on_draw(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4])
{
  uint16_t color = (rgba[0] << 8 & 0xf800) | (rgba[1] << 3 & 0x07e0) | (rgba[2] >> 3 & 0x001f);

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
  tft.fillRect(OFFSET_CAT_BY_X, OFFSET_CAT_BY_Y, 86, 99, COMMON_FILL_COLOR);
}

void draw_cat() {
  catSprite.createSprite(59, 99);
  catSprite.setColorDepth(16);
  catSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/One_hand_cat_59_99.png", &catSprite);
  catSprite.pushSprite(OFFSET_CAT_BY_X, OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
}

void draw_sunflower() {
  sunflowerSprite.createSprite(65, 120);
  sunflowerSprite.setColorDepth(16);
  sunflowerSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/Sunflower_65x120.png", &sunflowerSprite);
  sunflowerSprite.pushSprite(SUNFLOWER_X, SUNFLOWER_Y, TFT_TRANSPARENT);
}

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(1);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  tft.fillScreen(COMMON_FILL_COLOR);
  for (int i = 0; i < 320; i++){
    for (int j = 0; j < 29; j++){
      tft.drawPixel(i, j + 211, Soil_320_29[i][j]);
    }
  }

  // Create and load default hand sprite with transparent background
  defaultHandSprite.createSprite(DEFAULT_HAND_WIDTH, DEFAULT_HAND_HEIGHT);
  defaultHandSprite.setColorDepth(16);
  defaultHandSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/Default_hand_26_57.png", &defaultHandSprite);
  
  // Create and load attack hand sprite with transparent background
  attackHandSprite.createSprite(ATTACK_HAND_WIDTH, ATTACK_HAND_HEIGHT);
  attackHandSprite.setColorDepth(16);
  attackHandSprite.fillSprite(TFT_TRANSPARENT);
  setPngPosition(0, 0);
  load_image_into_sprite(SPIFFS, "/Attack_hand_38_53.png", &attackHandSprite);

  // Draw initial default hand
  draw_cat();
  defaultHandSprite.pushSprite(DEFAULT_HAND_X + OFFSET_CAT_BY_X, DEFAULT_HAND_Y + OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
  delay(750);
  // Draw the sunflower
  draw_sunflower();
}

unsigned long previousMillis = 0;
const long interval = 500;  // interval at which to change hand position (milliseconds)
bool isDefaultHand = true;  // track which hand is currently shown

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you changed the hand
    previousMillis = currentMillis;

    if (isDefaultHand) {
      // Change to attack hand
      erase_default_hand();
      attackHandSprite.pushSprite(ATTACK_HAND_X + OFFSET_CAT_BY_X, ATTACK_HAND_Y + OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
      isDefaultHand = false;
    } else {
      // Change to default hand
      erase_attack_hand();
      defaultHandSprite.pushSprite(DEFAULT_HAND_X + OFFSET_CAT_BY_X, DEFAULT_HAND_Y + OFFSET_CAT_BY_Y, TFT_TRANSPARENT);
      isDefaultHand = true;
    }
  }
}
