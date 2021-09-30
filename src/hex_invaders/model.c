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

// "position":
// number the characters of the displays, three 4-character displays, 0-11:
//
//  0  1  2  3  (green)
//  4  5  6  7  (blue)
//  8  9 10 11  (red)
//

struct laser {
  uint8_t value;  // hex value for player laser shot
  laser_state_t laser_state;
  uint8_t position;
};

struct player {
  uint8_t position;
  uint8_t shield;
  uint8_t score;
};

struct invader {
  invader_state_t invader_state;
  uint16_t glyph;
  uint8_t glyph_index;
  uint8_t position;
  uint8_t movement_direction;
  uint8_t invader_speed;
};

struct level {
  uint8_t level_number;
  uint8_t level_invader_speed;
  uint8_t remaining_invaders;
};

#define NUM_INVADERS 8
const static int num_invaders = NUM_INVADERS;

struct model {
  struct display_strategy *display_strategy;
  game_state_t game_state;
  struct level level;
  struct player player;
  struct laser laser;
  struct invader invaders[NUM_INVADERS];
};



static struct display_strategy* create_display_strategy(struct model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 

const static uint16_t invader_forming_glyphs[] =
  { 0x0040, 0x0100, 0x0200, 0x0400, 0x0080, 0x2000, 0x1000, 0x0800 };
   

/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));
  this->display_strategy = create_display_strategy(this);

  game_start(this);

  return this;
}


void game_start(struct model *this) {
  this->level.level_number = 1;
  this->level.level_invader_speed = 20;
  this->level.remaining_invaders = 8;
  this->player.position = 9;
  this->player.shield = 8;
  this->player.score = 0;
  this->laser.laser_state = READY;

  for (int i = 0; i < num_invaders; ++i) {
    this->invaders[i].invader_state = INACTIVE;
    this->invaders[i].position = 0;
    this->invaders[i].movement_direction = 1;
    this->invaders[i].invader_speed = this->level.level_invader_speed;
  }

  this->game_state = GAME_PLAYING;
}



void free_model(struct model *this) {
  free_display_strategy(this->display_strategy);
  return free(this);
}





static uint16_t player_ship_glyph = 0b0010100000001000;


// implements f_get_display for the red display
display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  *brightness = HT16K33_BRIGHTNESS_12;
  *blink = HT16K33_BLINK_OFF;

  if (this->game_state == GAME_PLAYING) {
    // clean existing display
    (*value).display_glyph[0] = 0;
    (*value).display_glyph[1] = 0;
    (*value).display_glyph[2] = 0;
    (*value).display_glyph[3] = 0;

    // player position is in the range 8-11.  Subtract 8 to get index to this
    (*value).display_glyph[this->player.position - 8] = player_ship_glyph;

    // draw any invaders on this display
    for (int i = 0; i < num_invaders; ++i) {
      if (this->invaders[i].invader_state != INACTIVE &&
	  this->invaders[i].position >= 8) {
	(*value).display_glyph[this->invaders[i].position - 8] =
	  this->invaders[i].glyph;
      }
    }

    return glyph_display;
  }
  else {
    (*value).display_string = "3";
    return string_display;
  }
}

// implements f_get_display for the blue display
display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  *brightness = HT16K33_BRIGHTNESS_12;
  *blink = HT16K33_BLINK_OFF;

  if (this->game_state == GAME_PLAYING) {
    // clean existing display
    (*value).display_glyph[0] = 0;
    (*value).display_glyph[1] = 0;
    (*value).display_glyph[2] = 0;
    (*value).display_glyph[3] = 0;
    // draw any invaders on this display
    for (int i = 0; i < num_invaders; ++i) {
      if (this->invaders[i].invader_state != INACTIVE &&
	  this->invaders[i].position >= 4 &&
	  this->invaders[i].position < 8) {
	(*value).display_glyph[this->invaders[i].position - 4] =
	  this->invaders[i].glyph;
      }
    }

    return glyph_display;
  }

  (*value).display_string = "2";

  return string_display;
}

// implements f_get_display for the green display
display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;


  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  if (this->game_state == GAME_PLAYING) {
    // clean existing display
    (*value).display_glyph[0] = 0;
    (*value).display_glyph[1] = 0;
    (*value).display_glyph[2] = 0;
    (*value).display_glyph[3] = 0;

    // draw any invaders on this display
    for (int i = 0; i < num_invaders; ++i) {
      if (this->invaders[i].invader_state != INACTIVE &&
	  this->invaders[i].position < 4) {
	(*value).display_glyph[this->invaders[i].position] =
	  this->invaders[i].glyph;
      }
    }

    return glyph_display;
  }


  (*value).display_string = "1";

  return string_display;
}




// implements f_get_display for the leds display
display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;
    
  (*value).display_int = 0;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}



struct display_strategy* get_display_strategy(struct model *this) {
  return this->display_strategy;
}



/** player_left player_right
 *
 * move the player left/right, if it's allowed
 */
void player_left(struct model *this) {
  if (this->game_state != GAME_PLAYING) {
    return;
  }

  if (this->player.position > 8) {
    this->player.position--;
  }
}

void player_right(struct model *this) {
  if (this->game_state != GAME_PLAYING) {
    return;
  }

  if (this->player.position < 11) {
    this->player.position++;
  }
}


void start_invader(struct model *this) {
  int i;

  // find next invader to start
  for (i = 0; i < num_invaders; ++i) {
    if (this->invaders[i].invader_state == INACTIVE) {
      this->invaders[i].invader_state = FORMING;
      this->invaders[i].glyph_index = 0;
      this->invaders[i].position = 3;
      return;
    }
  }
}

void update_invaders(struct model *this) {
  int i;

  for (i = 0; i < num_invaders; ++i) {
    if (this->invaders[i].invader_state == FORMING) {
      ++this->invaders[i].glyph_index;
      if (this->invaders[i].glyph_index ==
	  (sizeof(invader_forming_glyphs) / sizeof(uint16_t))) {
	this->invaders[i].glyph_index = 0;
      }
      this->invaders[i].glyph = invader_forming_glyphs[this->invaders[i].glyph_index];
    }
  }
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
