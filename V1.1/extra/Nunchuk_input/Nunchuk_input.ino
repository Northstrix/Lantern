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
#include <NintendoExtensionCtrl.h>

Nunchuk wii_nunchuk;

#define NUNCHUCK_SDA 22
#define NUNCHUCK_SCL 27

bool pressed_c = false;
bool pressed_z = false;
bool held_left = false;
bool held_up = false;
bool held_right = false;
bool held_down = false;
bool c_functions_as_enter = true;
bool invert_y_axis = false;
bool right_fast_scroll = false;
bool left_fast_scroll = false;
bool up_fast_scroll = false;
bool down_fast_scroll = false;
bool do_right_fast_scroll = false;
bool do_left_fast_scroll = false;
bool do_up_fast_scroll = false;
bool do_down_fast_scroll = false;

byte threshold = 16;
bool rec_d = false;
int wait_till_fast_scroll = 500;
int delay_for_fast_scroll = 50;

void setup() {
  Serial.begin(115200);
  Wire.begin(NUNCHUCK_SDA, NUNCHUCK_SCL);
  wii_nunchuk.begin();
  while (!wii_nunchuk.connect()) {
    Serial.println("Nunchuk not detected!");
    delay(1000);
  }
}

void loop() {
  byte input_data = get_nunchuk_input();
  if (input_data != 0) {
    Serial.println(input_data);
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
