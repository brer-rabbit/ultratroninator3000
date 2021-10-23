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


static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);


// view methods

/* view methods ----------------------------------------------------- */


struct display_strategy* create_playerhit_display_strategy(struct model *model) {
  struct display_strategy *display_strategy;
  display_strategy = (struct display_strategy*)malloc(sizeof(struct display_strategy));
  *display_strategy = (struct display_strategy const)
    {
     .userdata = model,
     .get_green_display = get_green_display,
     .green_blink = HT16K33_BLINK_OFF,
     .green_brightness = HT16K33_BRIGHTNESS_12,
     .get_blue_display = get_blue_display,
     .blue_blink = HT16K33_BLINK_OFF,
     .blue_brightness = HT16K33_BRIGHTNESS_12,
     .get_red_display = get_red_display,
     .red_blink = HT16K33_BLINK_OFF,
     .red_brightness = HT16K33_BRIGHTNESS_12,
     .get_leds_display = get_leds_display,
     .leds_blink = HT16K33_BLINK_OFF,
     .leds_brightness = HT16K33_BRIGHTNESS_12
    };

  return display_strategy;
}

void free_playerhit_display_strategy(struct display_strategy *display_strategy) {
  free(display_strategy);
}




// implements f_get_display for the red display
static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct player *player;

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  player = get_model_player(model);

  memset((*value).display_glyph, 0, sizeof(display_value));

  for (int i = 0; i < player->lives_remaining && i < 4; ++i) {
    (*value).display_glyph[i] = 0x2808;
  }

  return glyph_display;
}

// implements f_get_display for the blue display
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;
  (*value).display_glyph[0] = 0;
  (*value).display_glyph[1] = 0;
  (*value).display_glyph[2] = 0;
  (*value).display_glyph[3] = 0;

  return glyph_display;
}


// implements f_get_display for the green display
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct player_hit_and_restart *player_restart;

  player_restart = get_model_player_hit_and_restart(model);

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;
  (*value).display_string = player_restart->msg_ptr;

  return string_display;
}




// implements f_get_display for the leds display
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;

  (*value).display_int = 0;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}


