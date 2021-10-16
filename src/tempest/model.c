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
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "model.h"
#include "ut3k_pulseaudio.h"


static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);



struct player {
  int position;
};

// the playfield is a list of arrays...
// level zero being where the player is.  The greatest number (num_levels-1)
// is where the flippers originate

struct playfield {
  int num_levels;
  int max_index_per_level[6]; // assume we won't have more than size levels
  int **levels;
  int circular; // boolean- should it wrap around?
};

struct model {
  struct display_strategy *display_strategy;
  struct player player;
  struct playfield playfield;
};




static struct display_strategy* create_display_strategy(struct model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 



/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));
  this->display_strategy = create_display_strategy(this);

  this->player = (struct player const) { 0 };

  // just for now...get things going
  this->playfield = (struct playfield const)
    {
     .num_levels = 1,
     .levels = (int**)malloc(sizeof(int*) * 1),
     .max_index_per_level[0] = 28,
     .circular = 1
    };

  this->playfield.levels[0] = (int*)malloc(sizeof(int) * 28);

  return this;
}


void free_model(struct model *this) {
  for (int i = 0; i < this->playfield.num_levels; ++i) {
    free(this->playfield.levels[i]);
  }
  free(this->playfield.levels);

  free_display_strategy(this->display_strategy);
  return free(this);
}



void move_player(struct model *this, int8_t direction) {

  if (direction > 0) {
    this->player.position++;
    if (this->player.position >= this->playfield.max_index_per_level[0]) {
      this->player.position = this->playfield.circular ? 0 : this->playfield.max_index_per_level[0] - 1;
    }
  }
  else if (direction < 0) {
    this->player.position--;
    if (this->player.position < 0) {
      this->player.position = this->playfield.circular ?
	this->playfield.max_index_per_level[0] - 1 : 0;
    }
  }

  printf("player: %d\n", this->player.position);
}






struct display_strategy* get_display_strategy(struct model *this) {
  return this->display_strategy;
}






struct display_strategy* create_display_strategy(struct model *this) {
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



/* static ----------------------------------------------------------- */

static uint16_t player_glyph_red_blue[] =
   { 0x0160, 0x0850, 0x1800, 0x3000, 0x1800, 0x3000, 0x1800, 0x3000,
     0x1800, 0x3000, 0x2084, 0x0482 };

static uint16_t player_glyph_green[] =
  { 0x2084, 0x0482, 0x0600, 0x0300,  0x0600, 0x0300,  0x0600, 0x0300,
    0x0600, 0x0300, 0x0160, 0x0850 };


// implements f_get_display for the red display
static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  *brightness = HT16K33_BRIGHTNESS_12;
  *blink = HT16K33_BLINK_OFF;

  memset((*value).display_glyph, 0, sizeof(display_value));

  if (this->player.position < 4) {
    (*value).display_glyph[0] = player_glyph_red_blue[this->player.position];
  }
  else if (this->player.position < 6) {
    (*value).display_glyph[1] = player_glyph_red_blue[this->player.position];
  }  
  else if (this->player.position < 8) {
    (*value).display_glyph[2] = player_glyph_red_blue[this->player.position];
  }  
  else if (this->player.position < 12) {
    (*value).display_glyph[3] = player_glyph_red_blue[this->player.position];
  }  


  return glyph_display;
}

// implements f_get_display for the blue display
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  memset((*value).display_glyph, 0, sizeof(display_value));

  if (this->player.position > 11 && this->player.position < 14) {
    (*value).display_glyph[3] = player_glyph_red_blue[this->player.position - 2];
  }
  else if (this->player.position > 25) {
    (*value).display_glyph[0] = player_glyph_red_blue[this->player.position - 26];
  } 

  return glyph_display;
}

// implements f_get_display for the green display
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  memset((*value).display_glyph, 0, sizeof(display_value));

  if (this->player.position > 13 && this->player.position <= 17) {
    (*value).display_glyph[3] = player_glyph_green[this->player.position - 14];
  }
  else if (this->player.position > 17 && this->player.position <= 19) {
    (*value).display_glyph[2] = player_glyph_green[this->player.position - 14];
  } 
  else if (this->player.position > 19 && this->player.position <= 21) {
    (*value).display_glyph[1] = player_glyph_green[this->player.position - 14];
  } 
  else if (this->player.position > 21 && this->player.position <= 25) {
    (*value).display_glyph[0] = player_glyph_green[this->player.position - 14];
  } 

  return glyph_display;
}




// implements f_get_display for the leds display
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;
    
  (*value).display_int = 0;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}


