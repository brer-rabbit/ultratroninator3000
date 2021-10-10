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


/* initialize_model
 *
 * get the backpacks up and going. Clear their mem first.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "model.h"
#include "ut3k_pulseaudio.h"


struct model {
  struct display_strategy *display_strategy;

  // pack the instruments into a 2 byte structure.  if it's playing
  // on this step ( instrument_sequence[current_step] & {instr_mask} )
  // then output it
  uint16_t instrument_sequence[128];
  uint8_t current_step;
  uint8_t bpm;

  // from: https://www.kvraudio.com/forum/viewtopic.php?t=261858
  // Turns out that the shuffle of the TR-909 delays each even-numbered 1/16th by 2/96 of a beat for shuffle setting 1, 4/96 for 2, 6/96 for 3, 8/96 for 4, 10/96 for 5 and 12/96 for 6.
  uint8_t shuffle;
};



static struct display_strategy* create_display_strategy(struct model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 



/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));
  *this = (struct model const)
    {
     .display_strategy = create_display_strategy(this),
     .instrument_sequence = {0},
     .current_step = 0,
     .bpm = 120
    };

  return this;
}


void free_model(struct model *this) {
  free_display_strategy(this->display_strategy);
  return free(this);
}


void clocktick_model(struct model *this) {
  if (++this->current_step > 127) {
    this->current_step = 0;
  }
}

void set_bpm(struct model *this, int bpm) {
  this->bpm = bpm;
}


int get_bpm(struct model *this) {
  return this->bpm;
}

void change_bpm(struct model *this, int amount) {
  this->bpm += amount;
}


// implements f_get_display for the red display
display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  //(*value).display_int = ...

  (*value).display_string = "3";
  *brightness = HT16K33_BRIGHTNESS_12;
  *blink = HT16K33_BLINK_OFF;
  return string_display;
}

// implements f_get_display for the blue display
display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  (*value).display_string = "2";
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return string_display;
}

// implements f_get_display for the green display
display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  (*value).display_int = this->bpm;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}


static const uint32_t led_map[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

// implements f_get_display for the leds display
display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  if (this->current_step < 64) {
    // start on the green display for steps 1 - 8
    (*value).display_int = led_map[(this->current_step >> 3)] << 16;
  }
  else {
    // then move to blue display for steps 9 - 16
    (*value).display_int = led_map[((this->current_step >> 3) - 8)] << 8;
  }


  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}



struct display_strategy* get_display_strategy(struct model *this) {
  return this->display_strategy;
}



/* static ----------------------------------------------------------- */


static struct display_strategy* create_display_strategy(struct model *this) {
  struct display_strategy *display_strategy;
  display_strategy = (struct display_strategy*)malloc(sizeof(struct display_strategy));
  display_strategy->userdata = (void*)this;
  display_strategy->get_green_display = get_green_display;
  display_strategy->green_blink = HT16K33_BLINK_OFF;
  display_strategy->green_brightness = HT16K33_BRIGHTNESS_12;
  display_strategy->get_blue_display = get_blue_display;
  display_strategy->blue_blink = HT16K33_BLINK_OFF;
  display_strategy->blue_brightness = HT16K33_BRIGHTNESS_12;
  display_strategy->get_red_display = get_red_display;
  display_strategy->red_blink = HT16K33_BLINK_OFF;
  display_strategy->red_brightness = HT16K33_BRIGHTNESS_12;
  display_strategy->get_leds_display = get_leds_display;
  display_strategy->leds_blink = HT16K33_BLINK_OFF;
  display_strategy->leds_brightness = HT16K33_BRIGHTNESS_12;
  return display_strategy;
}

static void free_display_strategy(struct display_strategy *display_strategy) {
  free(display_strategy);
}
