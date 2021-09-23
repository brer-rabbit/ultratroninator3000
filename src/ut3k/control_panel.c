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


// Addresses
// Byte is offset from zero
// Bit is number of bits to shift right to shift it to least significant
static const int GREEN_ROTARY_ENCODER_BYTE = 5;
static const int GREEN_ROTARY_ENCODER_SIGA_BIT = 1;
static const int GREEN_ROTARY_ENCODER_PUSHBUTTON_BIT = 3;

static const int GREEN_SELECTOR_BYTE = 2;
static const int GREEN_SELECTOR_FIRST_BIT = 0;

static const int GREEN_BUTTON_BYTE = 2;
static const int GREEN_BUTTON_BIT = 0;


static const int BLUE_SELECTOR_BYTE = 3;
static const int BLUE_SELECTOR_FIRST_BIT = 0;

static const int BLUE_ROTARY_ENCODER_BYTE = 0;
static const int BLUE_ROTARY_ENCODER_SIGA_BIT = 1;
static const int BLUE_ROTARY_ENCODER_PUSHBUTTON_BIT = 3;

static const int BLUE_BUTTON_BYTE = 3;
static const int BLUE_BUTTON_BIT = 1;


static const int RED_ROTARY_ENCODER_BYTE = 0;
static const int RED_ROTARY_ENCODER_SIGA_BIT = 4;
static const int RED_ROTARY_ENCODER_PUSHBUTTON_BIT = 5;

static const int RED_BUTTON_BYTE = 6;
static const int RED_BUTTON_BIT = 0;


// index via previous_state<<2 | current_state.
// I tried many different mappings- this seemed to work best.
// Maybe the ht16k33 scan rate & debouncing are coming into play?
const static int encoder_lookup_table[] = {0,1,-1,0,0,0,0,0,0,0,0,0,0,-1,1,0};



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

  this->green_selector.selector_state = ZERO;
  this->green_selector.selector_previous_state = ZERO;
  this->green_selector.state_count = 0;

  this->green_button.button_previous_state = 0;
  this->green_button.button_state = 0;
  this->green_button.state_count = 0;

  return this;
}


void free_control_panel(struct control_panel *this) {
  free(this);
}


int update_control_panel(struct control_panel *this, ht16k33keyscan_t keyscan, uint32_t clock) {

  if ((ht16k33keyscan_byte(&keyscan, GREEN_BUTTON_BYTE) >> GREEN_BUTTON_BIT & 0b1) != this->green_button.button_state) {
    this->green_button.button_previous_state = this->green_button.button_state;
    this->green_button.button_state = ht16k33keyscan_byte(&keyscan, GREEN_BUTTON_BYTE) >> GREEN_BUTTON_BIT & 0b1;
    this->green_button.button_previous_state_count = this->green_button.state_count;
    this->green_button.state_count = 0;
  }
  else {
    this->green_button.state_count++;
  }


  return 0;
}



const struct button* get_green_button(struct control_panel *this) {
  return (const struct button*) &(this->green_button);
}
