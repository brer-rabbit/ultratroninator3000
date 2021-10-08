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


static const int JOYSTICK_BYTE0 = 0;
static const int JOYSTICK_BYTE1 = 1;
static const int JOYSTICK_PUSHBUTTON_BIT = 3;


static const int TOGGLES_BYTE = 4;

static void update_button(struct button *button, uint8_t value, uint32_t clock);
static void update_rotary_encoder(struct rotary_encoder *encoder, uint8_t value);
static void update_selector(struct selector *selector, uint8_t value);
static void update_toggles(struct toggles *toggles, uint8_t value);
static void update_joystick(struct joystick *joystick, uint8_t value);


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


struct control_panel* create_control_panel(ht16k33keyscan_t keyscan) {
  struct control_panel *this = (struct control_panel*) malloc(sizeof(struct control_panel));

  /* initialize stuff before processing keyscan that needs it,
   * then process the keyscan
   * finally set state to make it look like we're starting with a clean slate
   */

  this->green_selector.selector_state = SELECTOR_ZERO;
  this->green_selector.selector_previous_state = SELECTOR_INVALID;
  this->green_selector.state_count = 0;
  this->green_selector.selector_state_bits = 0;  // intentionally invalid

  this->blue_selector.selector_state = SELECTOR_ZERO;
  this->blue_selector.selector_previous_state = SELECTOR_INVALID;
  this->blue_selector.state_count = 0;
  this->blue_selector.selector_state_bits = 0;

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
  this->green_encoder.clock_ticks_to_neutral = 0;
  this->green_encoder.button.button_previous_state = 0;
  this->green_encoder.button.button_state = 0;
  this->green_encoder.button.state_count = 0;

  this->blue_encoder.encoder_delta = 0;
  this->blue_encoder.encoder_state = 0;
  this->blue_encoder.clock_ticks_to_neutral = 0;
  this->blue_encoder.button.button_previous_state = 0;
  this->blue_encoder.button.button_state = 0;
  this->blue_encoder.button.state_count = 0;

  this->red_encoder.encoder_delta = 0;
  this->red_encoder.encoder_state = 0;
  this->red_encoder.clock_ticks_to_neutral = 0;
  this->red_encoder.button.button_previous_state = 0;
  this->red_encoder.button.button_state = 0;
  this->red_encoder.button.state_count = 0;

  this->toggles.toggles_state = 0;
  this->toggles.toggles_previous_state = 0;
  this->toggles.toggles_toggled = 0;
  this->toggles.state_count = 0;

  update_control_panel(this, keyscan, 0);

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

  // the three encoders
  update_rotary_encoder(&(this->green_encoder), ht16k33keyscan_byte(&keyscan, GREEN_ROTARY_ENCODER_BYTE) >> GREEN_ROTARY_ENCODER_SIGA_BIT & 0b11);
  update_rotary_encoder(&(this->blue_encoder), ht16k33keyscan_byte(&keyscan, BLUE_ROTARY_ENCODER_BYTE) >> BLUE_ROTARY_ENCODER_SIGA_BIT & 0b11);
  update_rotary_encoder(&(this->red_encoder), ht16k33keyscan_byte(&keyscan, RED_ROTARY_ENCODER_BYTE) >> RED_ROTARY_ENCODER_SIGA_BIT & 0b11);

  // the two selectors
  update_selector(&(this->green_selector), ht16k33keyscan_byte(&keyscan, GREEN_SELECTOR_BYTE) >> GREEN_SELECTOR_FIRST_BIT & 0b1111);
  update_selector(&(this->blue_selector), ht16k33keyscan_byte(&keyscan, BLUE_SELECTOR_BYTE) >> BLUE_SELECTOR_FIRST_BIT & 0b1111);

  // toggle switches
  update_toggles(&(this->toggles), ht16k33keyscan_byte(&keyscan, TOGGLES_BYTE));

  // joystick pushbutton
  update_button(&(this->red_joystick.button), ht16k33keyscan_byte(&keyscan, JOYSTICK_BYTE1) >> JOYSTICK_PUSHBUTTON_BIT & 0b1, clock);

  // and finally the joystick
  // the joystick is 5 bits across two bytes.  Probably should have
  // thought of that before wiring everything up, but hey, here it is.
  update_joystick(&(this->red_joystick),
		  (ht16k33keyscan_byte(&keyscan, JOYSTICK_BYTE1) & 0b111) |
		  (ht16k33keyscan_byte(&keyscan, JOYSTICK_BYTE0) & 0b10000000));

  return 0;
}



const struct button* get_green_button(const struct control_panel *this) {
  return (const struct button*) &(this->green_button);
}

const struct button* get_blue_button(const struct control_panel *this) {
  return (const struct button*) &(this->blue_button);
}

const struct button* get_red_button(const struct control_panel *this) {
  return (const struct button*) &(this->red_button);
}



const struct rotary_encoder* get_green_rotary_encoder(const struct control_panel *this) {
  return (const struct rotary_encoder*) &(this->green_encoder);
}

const struct rotary_encoder* get_blue_rotary_encoder(const struct control_panel *this) {
    return (const struct rotary_encoder*) &(this->blue_encoder);
}

const struct rotary_encoder* get_red_rotary_encoder(const struct control_panel *this) {
  return (const struct rotary_encoder*) &(this->red_encoder);
}


const struct selector* get_green_selector(const struct control_panel *this) {
  return (const struct selector*) &(this->green_selector);
}

const struct selector* get_blue_selector(const struct control_panel *this) {
  return (const struct selector*) &(this->blue_selector);
}

const struct toggles* get_toggles(const struct control_panel *this) {
  return (const struct toggles*) &(this->toggles);
}

const struct joystick* get_joystick(const struct control_panel *this) {
  return (const struct joystick*) &(this->red_joystick);
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

// I tried many different mappings- this seemed to work best.  Part of
// the problem is the ht16k33 debounces and requires two stable
// readings.  Each keyscan reading is done once for 256us every 9.5ms.
// A lot of potentially missed rotary encoder states.  So to trigger a
// key we need the encoder to hold state for quite awhile.
//
// Complete total hack: use an expanded lookup table (statistically
// determined by lots of spinning slow, fast, medium).  Track current
// state and the previous two states; 6 bits total / 64 entries in
// table.  The additional state is useful for tracking faster
// movement.  Timeout and remove the upper two bits of state once
// movement stops/slows.


static int8_t encoder_lookup_table[] =
       {
           0,  // 0 000000
          -1,  // 1 000001 // last four bits are strong: don't care on upper 2
           1,  // 2 000010 // last four bits are strong: don't care on upper 2
           0,  // 3 000011
           0,  // 4 000100
           0,  // 5 000101
           0,  // 6 000110
           0,  // 7 000111
           0,  // 8 001000
           0,  // 9 001001
           0,  // 10 001010
           0,  // 11 001011
           0,  // 12 001100
           1,  // 13 001101
           0,  // 14 001110
           0,  // 15 001111

           0,  // 16 010000
          -1,  // 17 010001
           1,  // 18 010010
           0,  // 19 010011
           0,  // 20 010100
           0,  // 21 010101
           0,  // 22 010110
          -1,  // 23 010111
          -1,  // 24 011000
          -1,  // 25 011001
           0,  // 26 011010
           1,  // 27 011011
           0,  // 28 011100
          -1,  // 29 011101
           0,  // 30 011110
           0,  // 31 011111

           0,  // 32 100000
          -1,  // 33 100001
           1,  // 34 100010
           0,  // 35 100011
           0,  // 36 100100
           0,  // 37 100101
           0,  // 38 100110
          -1,  // 39 100111
           0,  // 40 101000
           1,  // 41 101001
           0,  // 42 101010
           1,  // 43 101011
           0,  // 44 101100
           1,  // 45 101101
           0,  // 46 101110
           0,  // 47 101111

           0,  // 48 110000
          -1,  // 49 110001
           1,  // 50 110010
           0,  // 51 110011
           0,  // 52 110100
           0,  // 53 110101
           0,  // 54 110110
          -1,  // 55 110111
          -1,  // 56 111000
          -1,  // 57 111001
           0,  // 58 111010
           1,  // 59 111011
           0,  // 60 111100
           0,  // 61 111101
           0,  // 62 111110
           0   // 63 111111
       }; 

const static int clock_ticks_for_temp_table = 6;


static void update_rotary_encoder(struct rotary_encoder *encoder, uint8_t value) {

  // technically this could underflow, but whatevz
  if (--encoder->clock_ticks_to_neutral <= 0) {
    // delete history minus most recent bits
    encoder->encoder_state = encoder->encoder_state & 0b11;
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
  encoder->encoder_state = encoder->encoder_state | value;

  switch (encoder_lookup_table[(encoder->encoder_state & 0b111111)]) {
  case -1:
    encoder->encoder_delta = -1;
    encoder->clock_ticks_to_neutral = clock_ticks_for_temp_table;
    break;
  case 1:
    encoder->encoder_delta = 1;
    encoder->clock_ticks_to_neutral = clock_ticks_for_temp_table;
    break;    
  case 0:
    encoder->encoder_delta = 0;
    break;
  }
}


/** update_selector
 * the funny thing about the selectors are they may make contact with
 * the second contact before disconnecting from the first.  make
 * before break.  account for this.  Assume that if more than one bit
 * is set, we're in transition to the next connection.  That is,
 * moving away from the previous connection.
 *
 * assumption: selector_previous_state only contains 1 bit set
 */

static void update_selector(struct selector *selector, uint8_t value) {

  if (value != selector->selector_state_bits) {
    // something changed
    // test that exactly one bit is set
    if (! (value && !(value & (value - 1)))) {
      // more than one bit is set - reset to only changed bits
      value = selector->selector_state_bits ^ value;
    }
    // else exactly one bit is set
    selector->selector_previous_state = selector->selector_state;
    selector->selector_state_bits = value;
    switch (value) {
    case 0b0001:
      selector->selector_state = SELECTOR_ZERO;
      break;
    case 0b0010:
      selector->selector_state = SELECTOR_ONE;
      break;
    case 0b0100:
      selector->selector_state = SELECTOR_TWO;
      break;
    case 0b1000:
      selector->selector_state = SELECTOR_THREE;
      break;
    default:
      selector->selector_state = SELECTOR_INVALID;
    }
    selector->state_count = 0;
  }
  else {
    selector->state_count++;
  }

}


static void update_toggles(struct toggles *toggles, uint8_t value) {
  if (value != toggles->toggles_state) {
    toggles->toggles_previous_state = toggles->toggles_state;
    toggles->toggles_state = value;
    toggles->toggles_toggled = toggles->toggles_state ^ toggles->toggles_previous_state;
    toggles->state_count = 0;
  }
  else {
    toggles->state_count++;
  }
}


static void update_joystick(struct joystick *joystick, uint8_t value) {
  if (joystick->previous_bits != value) {
    // something changed, assume it's a single bit and
    // that the joystick can only point one direction
    joystick->direction_previous = joystick->direction;
    joystick->state_count = 0;
    joystick->previous_bits = value;
    switch (value) {
    case 0b0000:
    default:
      joystick->direction = CENTERED;
      break;
    case 0b0001:
      joystick->direction = UP;
      break;
    case 0b0010:
      joystick->direction = DOWN;
      break;
    case 0b0100:
      joystick->direction = LEFT;
      break;
    case 0b10000000:
      joystick->direction = RIGHT;
      break;
    }
  }
  else {
    joystick->state_count++;
  }

}
