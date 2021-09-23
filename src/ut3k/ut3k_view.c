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

#include "ut3k_view.h"

#define GREEN_DISPLAY_ADDRESS HT16K33_ADDR_07
#define BLUE_DISPLAY_ADDRESS HT16K33_ADDR_06
#define RED_DISPLAY_ADDRESS HT16K33_ADDR_05
#define INPUTS_AND_LEDS_ADDRESS HT16K33_ADDR_04
#define I2C_ADAPTER_1 1

#define DISPLAY_GREEN 0
#define DISPLAY_BLUE 1
#define DISPLAY_RED 2
#define DISPLAY_ROWS 3

#define GREEN_ROTARY_ENCODER_BYTE 5
#define GREEN_ROTARY_ENCODER_PUSHBUTTON_BIT 3
#define GREEN_ROTARY_ENCODER_SIGA_BIT 1

#define BLUE_ROTARY_ENCODER_BYTE 0
#define BLUE_ROTARY_ENCODER_PUSHBUTTON_BIT 3
#define BLUE_ROTARY_ENCODER_SIGA_BIT 1

// index via previous_state<<2 | current_state.
// I tried many different mappings- this seemed to work best.
// Maybe the ht16k33 scan rate & debouncing are coming into play?
//const static int encoder_lookup_table[] = {0,1,-1,0,0,0,0,0,0,0,0,0,0,-1,1,0};
const static int encoder_lookup_table[] = {0,1,-1,0,0,0,0,0,0,0,0,0,0,-1,1,0};

struct rotary_encoder {
  f_controller_update_rotary_encoder callback;
  void *userdata;
  uint8_t button_previous_state;
  uint8_t button_state;
  uint8_t encoder_state;  // hold previous & current here, 4 bits
  uint8_t encoder_prev_state;  // hold previous & current here, 4 bits
  int encoder_ignore_count; // rotary encoder seems to work best if ignored for 1 read after a change
  int8_t encoder_delta;  // -1, 0, 1
};

static void read_rotary_encoder(struct rotary_encoder *encoder, uint8_t pushbutton, uint8_t rotary_encoder);


// I've no idea why this is parameterized here like this...why did I do that?
typedef void (*f_show_displays)(struct ut3k_view*, struct display_strategy*);

// implements f_show_displays
static void ht16k33_alphanum_display_game(struct ut3k_view *this, struct display_strategy *display);

struct ut3k_view {
  f_show_displays show_displays;

  // Adafruit displays with backpacks
  HT16K33 *green_display;
  HT16K33 *blue_display;
  HT16K33 *red_display;

  // aliases for the above
  HT16K33 *display_array[3];

  // bunch-o-knobs and the 3 sets of discrete LEDs are here
  HT16K33 *inputs_and_leds;

  struct rotary_encoder green_encoder_listener;
};




static int initialize_backpack(HT16K33 *backpack);



/** create_alphanum_ut3k_view
 * clients will want to call this function with a ut3k_view to construct
 * an alphanum view.
 * Creates a view that utilizes three of the Adafruit HT16K33 alphanum
 * displays
 */
struct ut3k_view* create_alphanum_ut3k_view() {
  struct ut3k_view *this;
  int rc = 0;

  this = (struct ut3k_view*) malloc(sizeof(struct ut3k_view));

  this->show_displays = ht16k33_alphanum_display_game;

  this->green_display = (HT16K33*) malloc(sizeof(HT16K33));
  this->blue_display  = (HT16K33*) malloc(sizeof(HT16K33));
  this->red_display   = (HT16K33*) malloc(sizeof(HT16K33));
  this->inputs_and_leds = (HT16K33*) malloc(sizeof(HT16K33));

  if (this->green_display == NULL ||
      this->blue_display == NULL ||
      this->red_display == NULL ||
      this->inputs_and_leds == NULL) {
    free(this);
    return NULL;
  }

  HT16K33_init(this->green_display, I2C_ADAPTER_1, GREEN_DISPLAY_ADDRESS);
  HT16K33_init(this->blue_display, I2C_ADAPTER_1, BLUE_DISPLAY_ADDRESS);
  HT16K33_init(this->red_display, I2C_ADAPTER_1, RED_DISPLAY_ADDRESS);
  HT16K33_init(this->inputs_and_leds, I2C_ADAPTER_1, INPUTS_AND_LEDS_ADDRESS);
  
  // alias in array
  this->display_array[DISPLAY_GREEN] = this->green_display;
  this->display_array[DISPLAY_BLUE] = this->blue_display;
  this->display_array[DISPLAY_RED] = this->red_display;

  // error codes positive...just sum them up

  rc += initialize_backpack(this->green_display);
  rc += initialize_backpack(this->blue_display);
  rc += initialize_backpack(this->red_display);
  rc += initialize_backpack(this->inputs_and_leds);

  if (rc) {
    free(this);
    return NULL;
  }
  
  // error codes negative
  rc = HT16K33_COMMIT(this->green_display);
  rc -= HT16K33_COMMIT(this->blue_display);
  rc -= HT16K33_COMMIT(this->red_display);
  rc -= HT16K33_COMMIT(this->inputs_and_leds);

  if (rc) {
    free(this);
    return NULL;
  }


  // clear all callbacks
  this->green_encoder_listener.callback = NULL;

  return this;
}


int free_ut3k_view(struct ut3k_view *this) {
  int rc = 0;
  if (this == NULL) {
    return 1;
  }

  HT16K33_CLOSE(this->green_display);
  HT16K33_CLOSE(this->blue_display);
  HT16K33_CLOSE(this->red_display);
  HT16K33_CLOSE(this->inputs_and_leds);

  free(this->green_display);
  free(this->blue_display);
  free(this->red_display);
  free(this->inputs_and_leds);

  free(this);
  return rc;

}


/** update_view
 * keyscan HT16K33
 * callback to any hooks
 * update displays
 */
void update_view(struct ut3k_view *this, struct display_strategy *display_strategy) {
  ht16k33keyscan_t keyscan;
  int keyscan_rc;


  keyscan_rc = HT16K33_READ(this->inputs_and_leds, keyscan);
  if (keyscan_rc != 0) {
    printf("keyscan failed with code %d\n", keyscan_rc);
  }

  //  printf("keyscan: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
  //  	 keyscan[0], keyscan[1], keyscan[2], keyscan[3], keyscan[4], keyscan[5]);

  // update control panel here...

  if (this->green_encoder_listener.callback) {
    uint8_t button_state_bit =
      (keyscan[GREEN_ROTARY_ENCODER_BYTE] >> GREEN_ROTARY_ENCODER_PUSHBUTTON_BIT) & 0b1;
    uint8_t rotary_encoder_bits =
      (keyscan[GREEN_ROTARY_ENCODER_BYTE] >> GREEN_ROTARY_ENCODER_SIGA_BIT) & 0b11;

    read_rotary_encoder(&this->green_encoder_listener, button_state_bit, rotary_encoder_bits);

    (*this->green_encoder_listener.callback)(this->green_encoder_listener.encoder_delta,
					     this->green_encoder_listener.button_state,
					     this->green_encoder_listener.button_state ^ this->green_encoder_listener.button_previous_state,
					     this->green_encoder_listener.userdata);

  }


  // odd..but abstract out implementation while passing object state.
  // maybe I should switch to an OO language?
  // show_displays is expected to update all visual info on the HT16K33s
  this->show_displays(this, display_strategy);
}




/* Listeners ---------------------------------------------------------- */

 /**
  * Register event handlers with the appropriate components.
  * Un(de?)register by calling with NULL values
  */
void register_green_encoder_listener(struct ut3k_view *view, f_controller_update_rotary_encoder f, void *userdata) {
  view->green_encoder_listener.callback = f;
  view->green_encoder_listener.userdata = userdata;
  view->green_encoder_listener.button_previous_state = 0;
  view->green_encoder_listener.button_state = 0;
  view->green_encoder_listener.encoder_state = 0;
  view->green_encoder_listener.encoder_prev_state = 0;
  view->green_encoder_listener.encoder_ignore_count = 0;
  view->green_encoder_listener.encoder_delta = 0;
}


/* Static ------------------------------------------------------------- */


/** ht16k33_alphanum_display_game: implements f_show_displays
 * specific implementation for the Adafruit alphanum display
 */

static void ht16k33_alphanum_display_game(struct ut3k_view *this, struct display_strategy *display_strategy) {
  int backpack, digit;
  display_value union_result;
  char *display[3];
  ht16k33blink_t blink;
  ht16k33brightness_t brightness;
  int led_display_value;
  

  switch(display_strategy->get_green_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
  case glyph_display:
    printf("integer and glyph not implemented\n");
    display[0] = NULL;
    break;
  case string_display:
    display[0] = union_result.display_string;
    break;
  }

  if (brightness != this->green_display->brightness) {
    HT16K33_BRIGHTNESS(this->green_display, brightness);
  }
  if (blink != this->green_display->blink_state) {
    HT16K33_BLINK(this->green_display, blink);
  }



  switch(display_strategy->get_blue_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
  case glyph_display:
    printf("integer and glyph not implemented\n");
    display[1] = NULL;
    break;
  case string_display:
    display[1] = union_result.display_string;
    break;
  }

  if (brightness != this->blue_display->brightness) {
    HT16K33_BRIGHTNESS(this->blue_display, brightness);
  }
  if (blink != this->blue_display->blink_state) {
    HT16K33_BLINK(this->blue_display, blink);
  }



  switch(display_strategy->get_red_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
  case glyph_display:
    printf("integer and glyph not implemented\n");
    display[2] = NULL;
    break;
  case string_display:
    display[2] = union_result.display_string;
    break;
  }


  if (brightness != this->red_display->brightness) {
    HT16K33_BRIGHTNESS(this->red_display, brightness);
  }
  if (blink != this->red_display->blink_state) {
    HT16K33_BLINK(this->red_display, blink);
  }


  for (backpack = 0; backpack < DISPLAY_ROWS; ++backpack) { // backpack

    for (digit = 0; digit < 4; ++digit) { // digit
      if (display[backpack] == NULL ||
	  display[backpack][digit] == '\0') {
	break;
      }
      // no colon sep on alphanum packs
      HT16K33_UPDATE_ALPHANUM(this->display_array[backpack], digit, display[backpack][digit], 0);

    }

    for (; digit < 4; ++digit) {
      // clear any remaining digits
      HT16K33_CLEAN_DIGIT(this->display_array[backpack], digit);
    }

    HT16K33_COMMIT(this->display_array[backpack]);
  }


  switch(display_strategy->get_leds_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
    led_display_value = union_result.display_int;
    break;
  case glyph_display:
  case string_display:
    printf("only integer supported\n");
    return;
  }

  if (brightness != this->inputs_and_leds->brightness) {
    HT16K33_BRIGHTNESS(this->inputs_and_leds, brightness);
  }
  if (blink != this->inputs_and_leds->blink_state) {
    HT16K33_BLINK(this->inputs_and_leds, blink);
  }

  HT16K33_UPDATE_RAW(this->inputs_and_leds, 4, (uint16_t)(led_display_value & 0x00FF));
  HT16K33_UPDATE_RAW(this->inputs_and_leds, 5, (uint16_t)((led_display_value >> 8) & 0x00FF));
  HT16K33_UPDATE_RAW(this->inputs_and_leds, 6, (uint16_t)((led_display_value >> 16) & 0x00FF));
  HT16K33_COMMIT(this->inputs_and_leds);

}





static int initialize_backpack(HT16K33 *backpack) {
  int rc;
  // prepare the backpack driver
  // (the first parameter is the raspberry pi i2c master controller attached to the HT16K33, the second is the i2c selection jumper)
  // The i2c selection address can be one of HT16K33_ADDR_01 to HT16K33_ADDR_08
  // initialize the backpack
  rc = HT16K33_OPEN(backpack);

  if(rc != 0) {
    fprintf(stderr, "Error initializing HT16K33 led backpack. Check your i2c bus (es. i2cdetect)\n");
    //fprintf(stderr, "Error initializing HT16K33 led backpack (%s). Check your i2c bus (es. i2cdetect)\n", strerror(backpack->lasterr));
    // you don't need to HT16K33_CLOSE() if HT16K33_OPEN() failed, but it's safe doing it.
    HT16K33_CLOSE(backpack);
    return rc;
  }

  rc = HT16K33_INTERRUPT(backpack, HT16K33_ROW15_INTERRUPT_HIGH);
  if (rc != 0) {
    fprintf(stderr, "Error putting the HT16K33 led backpack to ROW15 Driver. Check your i2c bus (es. i2cdetect)\n");
    // you don't need to HT16K33_OFF() if HT16K33_ON() failed, but it's safe doing it.
    HT16K33_OFF(backpack);
    HT16K33_CLOSE(backpack);
    return rc;
  }

  // power on the ht16k33
  rc = HT16K33_ON(backpack);
  if(rc != 0) {
    fprintf(stderr, "Error putting the HT16K33 led backpack 1 ON (%s). Check your i2c bus (es. i2cdetect)\n", strerror(backpack->lasterr));
    // you don't need to HT16K33_OFF() if HT16K33_ON() failed, but it's safe doing it.
    HT16K33_OFF(backpack);
    HT16K33_CLOSE(backpack);
    return rc;
  }

  // halfway bright
  HT16K33_BRIGHTNESS(backpack, HT16K33_BRIGHTNESS_7);

  // no blink
  HT16K33_BLINK(backpack, HT16K33_BLINK_OFF);

  // power on the display
  HT16K33_DISPLAY(backpack, HT16K33_DISPLAY_ON);

  return rc;
}


/* read_rotary_encoder
 * interpret the rotary encoder data and set fields on the
 * struct rotary_encoder
 *
 */
static void read_rotary_encoder(struct rotary_encoder *encoder, uint8_t pushbutton, uint8_t rotary_encoder) {
  uint8_t encoder_tmp_state = encoder->encoder_state << 2;

  // TODO: this needs some improvement.  Interface isn't quite as
  // fluid as it ought to be.

  // track encoder state even if we're ignoring the result
  // algorithm is influenced by
  // http://makeatronics.blogspot.com/2013/02/efficiently-reading-quadrature-with.html
  encoder_tmp_state = encoder_tmp_state | rotary_encoder;

  if (encoder_tmp_state != encoder->encoder_prev_state) {
    encoder->encoder_state = encoder_tmp_state & 0b1111;
    encoder->encoder_prev_state = encoder->encoder_state;
  }

  encoder->encoder_delta = encoder_lookup_table[encoder->encoder_state];

  // pushbutton
  encoder->button_previous_state = encoder->button_state;
  encoder->button_state = pushbutton;
}
