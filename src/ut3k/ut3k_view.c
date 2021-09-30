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
#include <unistd.h>

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

  struct control_panel *control_panel;
  void *control_panel_listener_userdata;  // for callback
  f_view_control_panel_listener control_panel_listener;  // the callback
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
  ht16k33keyscan_t keyscan;


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



  // get current state of control panel.  Some switches may
  // be set, so start off with the state reflecting that.
  rc = HT16K33_READ(this->inputs_and_leds, keyscan);
  if (rc != 0) {
    printf("keyscan failed with code %d\n", rc);
  }

  this->control_panel = create_control_panel(keyscan);

  return this;
}




int free_ut3k_view(struct ut3k_view *this) {
  int rc = 0;
  if (this == NULL) {
    return 1;
  }

  free_control_panel(this->control_panel);

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
void update_view(struct ut3k_view *this, struct display_strategy *display_strategy, uint32_t clock) {
  ht16k33keyscan_t keyscan;
  int keyscan_rc;


  keyscan_rc = HT16K33_READ(this->inputs_and_leds, keyscan);
  if (keyscan_rc != 0) {
    printf("keyscan failed with code %d\n", keyscan_rc);
  }

  //  printf("keyscan: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
  //  	 keyscan[0], keyscan[1], keyscan[2], keyscan[3], keyscan[4], keyscan[5]);

  // update control panel here...
  update_control_panel(this->control_panel, keyscan, clock);

  if (this->control_panel_listener) {
    (*this->control_panel_listener)((const struct control_panel*)this->control_panel, this->control_panel_listener_userdata);
  }


  // odd..but abstract out implementation while passing object state.
  // maybe I should switch to an OO language?
  // show_displays is expected to update all visual info on the HT16K33s
  this->show_displays(this, display_strategy);

}




/* Listeners ---------------------------------------------------------- */



void register_control_panel_listener(struct ut3k_view *view, f_view_control_panel_listener f, void *userdata) {
  view->control_panel_listener = f;
  view->control_panel_listener_userdata = userdata;
}


/* Accessors ---------------------------------------------------------- */

const struct control_panel* get_control_panel(struct ut3k_view *this) {
  return (const struct control_panel*) this->control_panel;
}



/* Static ------------------------------------------------------------- */


/** commit_string
 * 
 * write a string to a specific HT16K33 display.  Only the first 4
 * chars are written.  Commit is called.
 */
static void commit_string(HT16K33 *display, char *string) {
  int digit;

  for (digit = 0; digit < 4; ++digit) { // digit
    if (string == NULL || string[digit] == '\0') {
      break;
    }
    HT16K33_UPDATE_ALPHANUM(display, digit, string[digit], 0);
  }

  for (; digit < 4; ++digit) {
    // clear any remaining digits
    HT16K33_CLEAN_DIGIT(display, digit);
  }

  HT16K33_COMMIT(display);
}



/** commit_glyph
 *
 * raw read of four 16 bit ints sent straight to the update raw method.
 * more than enough rope to hang yourself with this.
 */
static void commit_glyph(HT16K33 *display, uint16_t glyph[]) {
  int digit;

  for (digit = 0; digit < 4; ++digit) { // digit
    HT16K33_UPDATE_RAW(display, digit, glyph[digit]);
  }

  HT16K33_COMMIT(display);
}



/** commit_integer
 * 
 * write a string to a specific HT16K33 display.  Only the first 4
 * chars are written.  Commit is called.
 */
static void commit_integer(HT16K33 *display, int16_t value) {

  if (value >= 0 && value < 256) {
    HT16K33_DISPLAY_INTEGER(display, (uint8_t)value);
  }
  else {
    char buffer[5];
    snprintf(buffer, 5, "%4hd", value);

    for (int digit = 0; digit < 4; ++digit) { // digit
      HT16K33_UPDATE_ALPHANUM(display, digit, buffer[digit], 0);
    }
  }

  HT16K33_COMMIT(display);
}





/** ht16k33_alphanum_display_game: implements f_show_displays
 * specific implementation for the Adafruit alphanum display
 */

static void ht16k33_alphanum_display_game(struct ut3k_view *this, struct display_strategy *display_strategy) {
  display_value union_result;
  ht16k33blink_t blink;
  ht16k33brightness_t brightness;
  int led_display_value;
  

  switch(display_strategy->get_green_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
    commit_integer(this->display_array[0], union_result.display_int);
    break;
  case glyph_display:
    commit_glyph(this->display_array[0], union_result.display_glyph);
    break;
  case string_display:
    commit_string(this->display_array[0], union_result.display_string);
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
    commit_integer(this->display_array[1], union_result.display_int);
    break;
  case glyph_display:
    commit_glyph(this->display_array[1], union_result.display_glyph);
    break;
  case string_display:
    commit_string(this->display_array[1], union_result.display_string);
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
    commit_integer(this->display_array[2], union_result.display_int);
    break;
  case glyph_display:
    commit_glyph(this->display_array[2], union_result.display_glyph);
    break;
  case string_display:
    commit_string(this->display_array[2], union_result.display_string);
    break;
  }


  if (brightness != this->red_display->brightness) {
    HT16K33_BRIGHTNESS(this->red_display, brightness);
  }
  if (blink != this->red_display->blink_state) {
    HT16K33_BLINK(this->red_display, blink);
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

