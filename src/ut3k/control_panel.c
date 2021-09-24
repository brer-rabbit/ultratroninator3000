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
static const int GREEN_SELECTOR_FIRST_BIT = 4;

static const int GREEN_BUTTON_BYTE = 2;
static const int GREEN_BUTTON_BIT = 0;


static const int BLUE_SELECTOR_BYTE = 3;
static const int BLUE_SELECTOR_FIRST_BIT = 0;

static const int BLUE_ROTARY_ENCODER_BYTE = 0;
static const int BLUE_ROTARY_ENCODER_SIGA_BIT = 1;
static const int BLUE_ROTARY_ENCODER_PUSHBUTTON_BIT = 3;

static const int BLUE_BUTTON_BYTE = 2;
static const int BLUE_BUTTON_BIT = 1;


static const int RED_ROTARY_ENCODER_BYTE = 0;
static const int RED_ROTARY_ENCODER_SIGA_BIT = 5;
static const int RED_ROTARY_ENCODER_PUSHBUTTON_BIT = 4;

static const int RED_BUTTON_BYTE = 5;
static const int RED_BUTTON_BIT = 0;


static const int JOYSTICK_BYTE = 1;
static const int JOYSTICK_PUSHBUTTON_BIT = 1;

static void update_button(struct button *button, uint8_t value, uint32_t clock);
static void update_rotary_encoder(struct rotary_encoder *encoder, uint8_t value);
static int encoder_lookup_table_neutral[] = {0,-1,1,0,0,0,0,0,0,0,0,0,0,0,-1,0};
static int encoder_lookup_table_ccw[] = {0,0,-1,-1,0,0,0,0,-1,0,0,0,0,-1,-1,0};
static int encoder_lookup_table_cw[] = {0,-1,1,1,1,0,0,0,0,0,0,0,0,1,1,0};
const static int clock_ticks_for_temp_table = 3;



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

  this->blue_button.button_previous_state = 0;
  this->blue_button.button_state = 0;
  this->blue_button.state_count = 0;

  this->red_button.button_previous_state = 0;
  this->red_button.button_state = 0;
  this->red_button.state_count = 0;

  this->red_joystick.button.button_previous_state = 0;
  this->red_joystick.button.button_state = 0;
  this->red_joystick.button.state_count = 0;

  this->green_encoder.encoder_delta = 0;
  this->green_encoder.encoder_state = 0;
  this->green_encoder.encoder_lookup_table = encoder_lookup_table_neutral;
  this->green_encoder.clock_ticks_to_neutral = 0;
  this->green_encoder.button.button_previous_state = 0;
  this->green_encoder.button.button_state = 0;
  this->green_encoder.button.state_count = 0;

  this->blue_encoder.encoder_delta = 0;
  this->blue_encoder.encoder_state = 0;
  this->blue_encoder.encoder_lookup_table = encoder_lookup_table_neutral;
  this->blue_encoder.clock_ticks_to_neutral = 0;
  this->blue_encoder.button.button_previous_state = 0;
  this->blue_encoder.button.button_state = 0;
  this->blue_encoder.button.state_count = 0;

  this->red_encoder.encoder_delta = 0;
  this->red_encoder.encoder_state = 0;
  this->red_encoder.encoder_lookup_table = encoder_lookup_table_neutral;
  this->red_encoder.clock_ticks_to_neutral = 0;
  this->red_encoder.button.button_previous_state = 0;
  this->red_encoder.button.button_state = 0;
  this->red_encoder.button.state_count = 0;




  return this;
}


void free_control_panel(struct control_panel *this) {
  free(this);
}






int update_control_panel(struct control_panel *this, ht16k33keyscan_t keyscan, uint32_t clock) {

  // Update buttons
  update_button(&(this->green_button), ht16k33keyscan_byte(&keyscan, GREEN_BUTTON_BYTE) >> GREEN_BUTTON_BIT & 0b1, clock);
  update_button(&(this->blue_button), ht16k33keyscan_byte(&keyscan, BLUE_BUTTON_BYTE) >> BLUE_BUTTON_BIT & 0b1, clock);
  update_button(&(this->red_button), ht16k33keyscan_byte(&keyscan, RED_BUTTON_BYTE) >> RED_BUTTON_BIT & 0b1, clock);

  // rotary encoder buttons are treated the same
  update_button(&(this->green_encoder.button), ht16k33keyscan_byte(&keyscan, GREEN_ROTARY_ENCODER_BYTE) >> GREEN_ROTARY_ENCODER_PUSHBUTTON_BIT & 0b1, clock);
  update_button(&(this->blue_encoder.button), ht16k33keyscan_byte(&keyscan, BLUE_ROTARY_ENCODER_BYTE) >> BLUE_ROTARY_ENCODER_PUSHBUTTON_BIT & 0b1, clock);
  update_button(&(this->red_encoder.button), ht16k33keyscan_byte(&keyscan, RED_ROTARY_ENCODER_BYTE) >> RED_ROTARY_ENCODER_PUSHBUTTON_BIT & 0b1, clock);

  // along with the joystick pushbutton
  update_button(&(this->red_joystick.button), ht16k33keyscan_byte(&keyscan, JOYSTICK_BYTE) >> JOYSTICK_PUSHBUTTON_BIT & 0b1, clock);

  // the three encoders
  update_rotary_encoder(&(this->green_encoder), ht16k33keyscan_byte(&keyscan, GREEN_ROTARY_ENCODER_BYTE) >> GREEN_ROTARY_ENCODER_SIGA_BIT & 0b11);
  update_rotary_encoder(&(this->blue_encoder), ht16k33keyscan_byte(&keyscan, BLUE_ROTARY_ENCODER_BYTE) >> BLUE_ROTARY_ENCODER_SIGA_BIT & 0b11);
  update_rotary_encoder(&(this->red_encoder), ht16k33keyscan_byte(&keyscan, RED_ROTARY_ENCODER_BYTE) >> RED_ROTARY_ENCODER_SIGA_BIT & 0b11);


  return 0;
}



const struct button* get_green_button(struct control_panel *this) {
  return (const struct button*) &(this->green_button);
}

const struct button* get_blue_button(struct control_panel *this) {
  return (const struct button*) &(this->blue_button);
}

const struct button* get_red_button(struct control_panel *this) {
  return (const struct button*) &(this->red_button);
}



const struct rotary_encoder* get_green_rotary_encoder(struct control_panel *this) {
  return (const struct rotary_encoder*) &(this->green_encoder);
}

const struct rotary_encoder* get_blue_rotary_encoder(struct control_panel *this) {
    return (const struct rotary_encoder*) &(this->blue_encoder);
}

const struct rotary_encoder* get_red_rotary_encoder(struct control_panel *this) {
  return (const struct rotary_encoder*) &(this->red_encoder);
}




/* internal functions ***************************************************/



static void update_button(struct button *button, uint8_t value, uint32_t clock) {
  if (value != button->button_state) {
    // update previous from what was current
    button->button_previous_state = button->button_state;
    // set current
    button->button_state = value;
    button->button_previous_state_count = button->state_count;
    button->state_count = 0;
  }
  else {
    button->state_count++;
  }

}




// index via previous_state<<2 | current_state.
// I tried many different mappings- this seemed to work best.
// Part of the problem is the ht16k33 debounces and requires two
// stable readings.  The scan rate is ~20 ms.  Too slow to read
// every single transition from rotary encoders.

// Complete total hack: use a modified lookup table (statistically
// determined by lots of spinning slow, fast, medium).  If we find
// the encoder is moving CW or CCW, switch to a direction-specific
// table for one or two reads.  Each subsequent move resets the timer
// on the direction-specific table.
// The direction specific tables allow for some invalid states to register
// as legit.  Total hack, but it works quite nice with this particular
// keyscan chip.


static void update_rotary_encoder(struct rotary_encoder *encoder, uint8_t value) {

  // technically this could underflow, but whatevz
  if (--encoder->clock_ticks_to_neutral <= 0) {
    encoder->encoder_lookup_table = encoder_lookup_table_neutral;
  }

  // first check to see if the bits are the same or different
  if ((encoder->encoder_state & 0b11) == value) {
    // they are the same -- no changes
    encoder->encoder_delta = 0;
    return;
  }

  // else something changed.
  // move encoder state up two bits
  encoder->encoder_state = encoder->encoder_state << 2;
  encoder->encoder_state = (encoder->encoder_state | value) & 0b1111;

  switch (encoder->encoder_lookup_table[encoder->encoder_state]) {
  case -1:
    encoder->encoder_delta = -1;
    encoder->encoder_lookup_table = encoder_lookup_table_ccw;
    encoder->clock_ticks_to_neutral = clock_ticks_for_temp_table;
    break;
  case 1:
    encoder->encoder_delta = 1;
    encoder->encoder_lookup_table = encoder_lookup_table_cw;
    encoder->clock_ticks_to_neutral = clock_ticks_for_temp_table;
    break;    
  case 0:
    encoder->encoder_delta = 0;
    break;
  }
}



