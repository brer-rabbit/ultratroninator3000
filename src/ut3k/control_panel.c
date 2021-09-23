/* Copyright 2021 Kyle Farrell
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control_panel.h"


// a basic button
struct button {
  uint8_t button_previous_state; // what was it
  uint8_t button_state; // what is it
  uint32_t state_count; // number of cycles in this state: 0 means toggled: it only goes up from there!
};


// these rotary encoders include a push button
struct rotary_encoder {
  uint8_t encoder_state;  // hold previous & current here, 4 bits
  int8_t encoder_delta;  // -1, 0, 1: what you're probably most interested in

  struct button button;  // the press button
};



// selector has one of four values
enum selector_value { ZERO = 0, ONE = 1, TWO = 2, THREE = 3 };
struct selector {
  enum selector_value selector_state;
  enum selector_value selector_previous_state;
  uint32_t state_count; // number of cycles in this state: 0 means toggled
};


// joystick is only a single direction and includes a pushbutton
enum direction { UP, DOWN, LEFT, RIGHT, CENTERED };
struct joystick {
  enum direction direction;
  enum direction direction_previous;
  uint32_t state_count;
  struct button button;
};


// toggle switches we do as a single byte
struct toggles {
  uint8_t toggles_state;
  uint8_t toggles_previous_state;
  uint8_t toggles_toggled;  // never thought I'd write that
};

struct control_panel {
  struct selector green_selector;
  struct rotary_encoder green_encoder;
  struct button green_button;

  struct rotary_encoder blue_encoder;
  struct selector blue_selector;
  struct button blue_button;

  struct joystick red_joystick;
  struct rotary_encoder red_encoder;
  struct button red_button;

  struct toggles toggles;
};


struct control_panel* create_control_panel() {
  struct control_panel *this = (struct control_panel*) malloc(sizeof(struct control_panel));

  return this;
}
