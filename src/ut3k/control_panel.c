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
static const int GREEN_ROTARY_ENCODER_PUSHBUTTON_BIT = 3;

static const int GREEN_SELECTOR_BYTE = 2;
static const int GREEN_SELECTOR_FIRST_BIT = 4;

static const int GREEN_BUTTON_BYTE = 2;
static const int GREEN_BUTTON_BIT = 0;


static const int BLUE_SELECTOR_BYTE = 3;
static const int BLUE_SELECTOR_FIRST_BIT = 0;

static const int BLUE_ROTARY_ENCODER_BYTE = 0;
static const int BLUE_ROTARY_ENCODER_PUSHBUTTON_BIT = 3;

static const int BLUE_BUTTON_BYTE = 2;
static const int BLUE_BUTTON_BIT = 1;

static const int RED_ROTARY_ENCODER_BYTE = 0;
static const int RED_ROTARY_ENCODER_PUSHBUTTON_BIT = 4;

static const int RED_BUTTON_BYTE = 5;
static const int RED_BUTTON_BIT = 0;

static const int JOYSTICK_BYTE0 = 0;
static const int JOYSTICK_BYTE1 = 1;
static const int JOYSTICK_PUSHBUTTON_BIT = 3;


static const int TOGGLES_BYTE = 4;

static void update_button(struct button *button, uint8_t value, uint32_t clock);
static void update_rotary_encoder(struct rotary_encoder *encoder, unsigned long long int bit_queue, int queue_index);
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

  this->green_selector = (struct selector const)
    {
     .selector_state = SELECTOR_ZERO,
     .selector_previous_state = SELECTOR_INVALID,
     .state_count = 0,
     .selector_state_bits = 0  // intentionally invalid
    };

  this->blue_selector = (struct selector const)
    {
     .selector_state = SELECTOR_ZERO,
     .selector_previous_state = SELECTOR_INVALID,
     .state_count = 0,
     .selector_state_bits = 0  // intentionally invalid
    };

  this->green_button = (struct button const) { 0 };
  this->blue_button = (struct button const) { 0 };
  this->red_button = (struct button const) { 0 };

  this->red_joystick = (struct joystick const)
    {
     .direction = CENTERED,
     .direction_previous = CENTERED,
     .state_count = 0,
     .previous_bits = 0,
     .button = { 0 }
    };

  this->green_encoder = (struct rotary_encoder const) { 0 };
  this->blue_encoder = (struct rotary_encoder const) { 0 };
  this->red_encoder = (struct rotary_encoder const) { 0 };

  this->toggles = (struct toggles const) { 0 };

  // this is mainly for more stateful input devices:
  // toggle switches, selectors
  update_control_panel(this, keyscan, 0, 0, 0, 0, 0, 0, 0);

  return this;
}


void free_control_panel(struct control_panel *this) {
  free(this);
}





int update_control_panel(struct control_panel *this,
			 ht16k33keyscan_t keyscan,
			 unsigned long long int green_bit_queue,
			 int green_queue_index,
			 unsigned long long int blue_bit_queue,
			 int blue_queue_index,
			 unsigned long long int red_bit_queue,
			 int red_queue_index,
			 uint32_t clock) {
  // Update buttons
  update_button(&(this->green_button), ht16k33keyscan_byte(&keyscan, GREEN_BUTTON_BYTE) >> GREEN_BUTTON_BIT & 0b1, clock);
  update_button(&(this->blue_button), ht16k33keyscan_byte(&keyscan, BLUE_BUTTON_BYTE) >> BLUE_BUTTON_BIT & 0b1, clock);
  update_button(&(this->red_button), ht16k33keyscan_byte(&keyscan, RED_BUTTON_BYTE) >> RED_BUTTON_BIT & 0b1, clock);

  // rotary encoder buttons are treated the same
  update_button(&(this->green_encoder.button), ht16k33keyscan_byte(&keyscan, GREEN_ROTARY_ENCODER_BYTE) >> GREEN_ROTARY_ENCODER_PUSHBUTTON_BIT & 0b1, clock);
  update_button(&(this->blue_encoder.button), ht16k33keyscan_byte(&keyscan, BLUE_ROTARY_ENCODER_BYTE) >> BLUE_ROTARY_ENCODER_PUSHBUTTON_BIT & 0b1, clock);
  update_button(&(this->red_encoder.button), ht16k33keyscan_byte(&keyscan, RED_ROTARY_ENCODER_BYTE) >> RED_ROTARY_ENCODER_PUSHBUTTON_BIT & 0b1, clock);

  // the three encoders
  update_rotary_encoder(&(this->green_encoder), green_bit_queue, green_queue_index);
  update_rotary_encoder(&(this->blue_encoder), blue_bit_queue, blue_queue_index);
  update_rotary_encoder(&(this->red_encoder), red_bit_queue, red_queue_index);

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



// rotary encoders: out with the HT16K33, rewired the internals to use
// the Raspberry Pi directly.  And whatdya know, 20ms between the
// HT16K33 reads was definitely not enough.  Anyway, that changes the
// interface here a bit.  The update call takes a queue (reference
// ut3k_view.c update_rotary_encoder) of B,A values readings from the
// encoder.  This function replays the history, draining the queue.
//
// Provided the queue has at least 4 bits (two samples/reading of A/B)
// grab the oldest 4 bits.  Do a table lookup.  Shift the 4-bit window
// by 2 to more recent A/B samples.  Accumulate values this way, record
// an actual delta when state of AB == 11 is reached.  The accumulation
// is persistent in the rotary_encoder struct, so the next reading may
// progress the accumulation value.
// 

static const int lookup_table[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};


static void update_rotary_encoder(struct rotary_encoder *encoder, unsigned long long int bit_queue, int queue_index) {
  uint8_t encoder_state;

  encoder->encoder_delta = 0;

  // process the queue.  Producing a delta requires getting a couple
  // samples in the 20ms loop.
  // loop while we have at least 4 bits
  while (queue_index > 3) {
    encoder_state = (bit_queue >> (queue_index - 4)) & 0b1111;
    encoder->accumulator += lookup_table[encoder_state];
    queue_index -= 2;

    if ((encoder_state & 0b11) == 0b11) {
      encoder->encoder_delta += encoder->accumulator > 0 ? 1 : -1;
      encoder->accumulator = 0;
    }

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
