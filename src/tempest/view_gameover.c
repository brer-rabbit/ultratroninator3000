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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "tempest.h"
#include "model.h"
#include "view_gameover.h"


static display_type_t get_red_display(struct display_strategy *display_strategy, display_value_t *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type_t get_blue_display(struct display_strategy *display_strategy, display_value_t *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type_t get_green_display(struct display_strategy *display_strategy, display_value_t *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type_t get_leds_display(struct display_strategy *display_strategy, display_value_t *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);



/* view methods ----------------------------------------------------- */


struct display_strategy* create_gameover_display_strategy(struct model *model) {
  struct display_strategy *display_strategy;
  display_strategy = (struct display_strategy*)malloc(sizeof(struct display_strategy));
  *display_strategy = (struct display_strategy const)
    {
     .userdata = model,
     .get_green_display = get_green_display,
     .green_blink = HT16K33_BLINK_OFF,
     .green_brightness = HT16K33_BRIGHTNESS_5,
     .get_blue_display = get_blue_display,
     .blue_blink = HT16K33_BLINK_OFF,
     .blue_brightness = HT16K33_BRIGHTNESS_5,
     .get_red_display = get_red_display,
     .red_blink = HT16K33_BLINK_OFF,
     .red_brightness = HT16K33_BRIGHTNESS_5,
     .get_leds_display = get_leds_display,
     .leds_blink = HT16K33_BLINK_OFF,
     .leds_brightness = HT16K33_BRIGHTNESS_5
    };


  return display_strategy;
}

void free_gameover_display_strategy(struct display_strategy *display_strategy) {
  free(display_strategy);
}




// implements f_get_display for the red display
static display_type_t get_red_display(struct display_strategy *display_strategy, display_value_t *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct gameover *gameover = get_model_gameover(model);

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_5;

  (*value).display_string = gameover->level_msg_ptr;

  return string_display;
}

// implements f_get_display for the blue display
static display_type_t get_blue_display(struct display_strategy *display_strategy, display_value_t *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct gameover *gameover = get_model_gameover(model);

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_5;

  (*value).display_string = gameover->score_msg_ptr;

  return string_display;
}


// implements f_get_display for the green display
static display_type_t get_green_display(struct display_strategy *display_strategy, display_value_t *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct gameover *gameover = get_model_gameover(model);

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_5;

  memset((*value).display_glyph, 0, sizeof(display_value_t));

  switch (gameover->animation_timer) {
  case 1:
    (*value).display_glyph[0] = 1 << rand() % 16;
    break;
  case 3:
    (*value).display_glyph[1] = 1 << rand() % 16;
    break;
  case 5:
    (*value).display_glyph[2] = 1 << rand() % 16;
    break;
  case 8:
    (*value).display_glyph[3] = 1 << rand() % 16;
    break;
  default:
    break;
  }

  return glyph_display;
}


static const int light_show[] = { 0x00, 0x81, 0xC3, 0xE7, 0xFF };

// implements f_get_display for the leds display
static display_type_t get_leds_display(struct display_strategy *display_strategy, display_value_t *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct gameover *gameover = get_model_gameover(model);

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  (*value).display_int =
    light_show[gameover->led_timer % 5] |
    (light_show[(gameover->led_timer + 1) % 5]) << 8 |
    (light_show[(gameover->led_timer + 2) % 5]) << 16;


  return integer_display;
}


