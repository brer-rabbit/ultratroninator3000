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


/* control_panel.h
 *
 */

#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include <stdint.h>

#include "ht16k33.h"


// ok, wierd hybrid of data hiding and exposure here.  The control_panel
// is an opaque struct.  The various "get" methods return non-opaque
// data defined below (button, selectors, etc).

struct control_panel;


// a basic button
struct button {
  uint8_t button_state; // what is it
  uint32_t state_count; // number of cycles in this state: 0 means toggled: it only goes up from there!
  uint8_t button_previous_state; // what was it
  uint32_t button_previous_state_count; // how long was it pressed before changing
};



// these rotary encoders include a push button
struct rotary_encoder {
  int8_t encoder_delta;  // -1, 0, 1: what you're probably most interested in

  struct button button;  // the press button

  // guru meditation: these ought to be hidden from the user, they're
  // implementation details
  uint8_t encoder_state;  // hold previous & current here, 4 bits
  int *encoder_lookup_table;
  int clock_ticks_to_neutral;
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
  uint32_t state_count;
};


/** create_control_panel
 *
 * construct a control panel.  This object is reponsible for state of
 * all user interface gadgets on the front panel.
 */
struct control_panel* create_control_panel();


/** free any resources allocated for the control panel
 */
void free_control_panel(struct control_panel *this);


/** update_control_panel
 *
 * take the 6 bytes from the HT16K33 and map it to something
 * interpretable in terms of buttons and cool stuff.
 * this function is expected to be called at regular intervals:
 * the counts that are provided in the various accessor methods
 * are based upon calls to update_panel to increment the counts.
 */
int update_control_panel(struct control_panel *this, ht16k33keyscan_t keyscan, uint32_t clock);


/** get_green_button
 * get the green button state.
 * Caller gets a pointer to a struct back that should not be modified
 * or free'd.
 */
const struct button* get_green_button(struct control_panel *this);
const struct button* get_blue_button(struct control_panel *this);
const struct button* get_red_button(struct control_panel *this);


/** get encoders
 * green blue red
 */
const struct rotary_encoder* get_green_rotary_encoder(struct control_panel *this);
const struct rotary_encoder* get_blue_rotary_encoder(struct control_panel *this);
const struct rotary_encoder* get_red_rotary_encoder(struct control_panel *this);


/** get_green_selector
 * get the change in the selector value and it's button state
 */
void get_green_selector(struct control_panel *this, int *encoder_delta, int *button_state);


#endif // CONTROL_PANEL_H
