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


/* initialize_calc_model
 *
 * get the backpacks up and going. Clear their mem first.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "calc_model.h"


struct calc_model {
  uint8_t green_register;
  uint8_t blue_register;
  uint8_t red_register;
  uint32_t red_register_uint32;
  f_calc f_calculator;
  f_next_value f_next_value_blue;


  struct display_strategy *display_strategy;
};



static struct display_strategy* create_display_strategy(struct calc_model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 

// Four different f_calc implementations
uint32_t f_calc_add(uint8_t a, uint8_t b) {
  return a+b;
}

uint32_t f_calc_sub(uint8_t a, uint8_t b) {
  return a-b;
}

uint32_t f_calc_or(uint8_t a, uint8_t b) {
  return a|b;
}

uint32_t f_calc_and(uint8_t a, uint8_t b) {
  return a&b;
}


// four f_next_value implementations
uint8_t f_next_plus_one(uint8_t a) {
  return a + 1;
}
uint8_t f_next_minus_one(uint8_t a) {
  return a - 1;
}
uint8_t f_next_times_two(uint8_t a) {
  return ((uint8_t)(a * 2)) == 0 ? 1 : a * 2;
}
uint8_t f_next_random(uint8_t a) {
  return (uint8_t)rand();
}


/** create the model.
 */
struct calc_model* create_calc_model() {
  struct calc_model* this = (struct calc_model*)malloc(sizeof(struct calc_model));

  this->green_register = rand() % UINT8_MAX;
  this->blue_register = rand() % UINT8_MAX;
  this->f_calculator = f_calc_add;
  this->f_next_value_blue = f_next_plus_one;
  this->red_register = (this->f_calculator)(this->green_register, this->blue_register);

  this->display_strategy = create_display_strategy(this);
  return this;
}


void free_calc_model(struct calc_model *this) {
  free_display_strategy(this->display_strategy);
  return free(this);
}


void set_calc_function(struct calc_model *this, f_calc f_calculator) {
  this->f_calculator = f_calculator;
}

void set_next_value_blue_function(struct calc_model *this, f_next_value f_next_value_func) {
  this->f_next_value_blue = f_next_value_func;
}


void update_green_integer(struct calc_model *this) {
  this->green_register++;
}

void update_blue_integer(struct calc_model *this) {
  this->blue_register = this->f_next_value_blue(this->blue_register);
}

void update_red_integer(struct calc_model *this) {
  this->red_register_uint32 = (this->f_calculator)(this->green_register, this->blue_register);
  this->red_register = this->red_register_uint32;
}


int32_t get_green_integer(struct calc_model *this) {
  if (this) {
    return this->green_register;
  }
  return 0;
}

int32_t get_blue_integer(struct calc_model *this) {
  if (this) {
    return this->blue_register;
  }
  return 0;
}

int32_t get_red_integer(struct calc_model *this) {
  if (this) {
    return this->red_register;
  }
  return 0;
}


// implements f_get_display for the red display
display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct calc_model *this = (struct calc_model*) display_strategy->userdata;

  (*value).display_int = this->red_register;

  *brightness = HT16K33_BRIGHTNESS_12;

  if (this->red_register_uint32 > this->red_register) {
    *blink = HT16K33_BLINK_FAST;
  }
  else {
    *blink = HT16K33_BLINK_OFF;
  }
    


  return integer_display;
}

// implements f_get_display for the blue display
display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct calc_model *this = (struct calc_model*) display_strategy->userdata;

  uint8_t blue_register = this->blue_register;
  (*value).display_int = blue_register;

  // no change
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}

// implements f_get_display for the green display
display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct calc_model *this = (struct calc_model*) display_strategy->userdata;

  uint8_t green_register = this->green_register;
  (*value).display_int = green_register;

  // no change
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}




// implements f_get_display for the leds display
display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct calc_model *this = (struct calc_model*) display_strategy->userdata;
    
  (*value).display_int =
    (get_green_integer(this) << 16) |
    (get_blue_integer(this) << 8) |
    (get_red_integer(this));

  // no change
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}



struct display_strategy* get_display_strategy(struct calc_model *this) {
  return this->display_strategy;
}



/* static ----------------------------------------------------------- */


static struct display_strategy* create_display_strategy(struct calc_model *this) {
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
