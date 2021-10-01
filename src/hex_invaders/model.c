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

typedef enum { GAME_ATTRACT, GAME_OVER, GAME_PLAYING } game_state_t;
typedef enum { FIRED, READY } laser_state_t;
typedef enum { CHARGING, DRAINING } player_laser_state_t;
typedef enum { FORMING, ACTIVE, INACTIVE } invader_state_t;



// "position":
// number the characters of the displays, three 4-character displays, 0-11:
//
//  0  1  2  3  (green)
//  4  5  6  7  (blue)
//  8  9 10 11  (red)
//
static uint8_t invaders_marching_orders[] = { 0, 1, 2, 3, 7, 6, 5, 4, 8, 9, 10, 11 };

const static uint8_t laser_fired_clockticks = 4;
const static uint8_t laser_charge_clockticks = 8;
const static uint8_t laser_drain_clockticks = 64;

struct laser {
  uint8_t value;  // hex value for player laser shot
  laser_state_t laser_state;
  uint8_t laser_state_clockticks;
  uint8_t position;
  uint16_t glyph;
};

struct player {
  uint8_t position;
  uint8_t shield;  // "lives left", display on green LEDs
  uint8_t score;
  uint8_t laser_value; // the value that will be fired from the laser
  uint16_t glyph;
  player_laser_state_t laser_charge_state;
  uint8_t player_laser_state_clockticks;
  uint8_t laser_charge; // how much charge does it have, displayed on blue LEDs
};

struct invader {
  invader_state_t invader_state;
  uint16_t glyph;
  uint8_t glyph_index;
  uint8_t hex_value;
  uint8_t position; // stpes mapped via table to LED digit position
  uint8_t steps;  // how many "steps" it's taken
};

struct level {
  uint8_t level_number;
  uint8_t level_invader_speed;
  uint8_t level_invader_speed_clock;
  uint8_t total_invaders;
  uint8_t remaining_invaders_to_form;
};

#define NUM_INVADERS 8
const static int num_invaders = NUM_INVADERS;

struct model {
  struct display_strategy *display_strategy;
  game_state_t game_state;
  struct level level;
  struct player player;
  struct laser laser;
  struct invader invaders[NUM_INVADERS];  // oh C...must I use a define?
};



static struct display_strategy* create_display_strategy(struct model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 

static uint16_t player_ship_glyph = 0b0010100000001000;

static uint16_t laser_high_glyph = 0b0000001000000000;
static uint16_t laser_low_glyph = 0b0001000000000000;

const static uint16_t invader_forming_glyphs[] =
  { 0x0040, 0x0100, 0x0200, 0x0400, 0x0080, 0x2000, 0x1000, 0x0800 };
   
const static uint16_t invader_hex_glyphs[] =
  {
   0b0000110000111111, // 0
   0b0000000000000110, // 1
   0b0000000011011011, // 2
   0b0000000010001111, // 3
   0b0000000011100110, // 4
   0b0010000001101001, // 5
   0b0000000011111101, // 6
   0b0000000000000111, // 7
   0b0000000011111111, // 8
   0b0000000011101111, // 9
   0b0000000011110111, // A
   0b0001001010001111, // B
   0b0000000000111001, // C
   0b0001001000001111, // D
   0b0000000011111001, // E
   0b0000000001110001  // F
  };


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
  this->level.level_invader_speed = 255;
  this->level.level_invader_speed_clock = this->level.level_invader_speed;
  this->level.remaining_invaders_to_form = 8;
  this->level.total_invaders = this->level.remaining_invaders_to_form;
  this->player.position = 9;
  this->player.shield = 0xFF;
  this->player.score = 0;
  this->player.glyph = player_ship_glyph;
  this->player.laser_charge = 0xFF;
  this->player.laser_charge_state = CHARGING;
  this->player.player_laser_state_clockticks = laser_charge_clockticks;
  this->laser.laser_state = READY;

  for (int i = 0; i < num_invaders; ++i) {
    this->invaders[i].invader_state = INACTIVE;
    this->invaders[i].position = 0;
    this->invaders[i].steps = 0;
  }

  this->game_state = GAME_PLAYING;
}


void game_over(struct model *this) {
  this->game_state = GAME_OVER;
  // show score, play game over sound
}


void game_attract(struct model *this) {
  this->game_state = GAME_ATTRACT;
}



void free_model(struct model *this) {
  free_display_strategy(this->display_strategy);
  return free(this);
}


  




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
    (*value).display_glyph[this->player.position - 8] = this->player.glyph;
    // if laser is here draw it too
    if (this->laser.laser_state == FIRED && this->laser.position > 7) {
      (*value).display_glyph[this->laser.position - 8] |= this->laser.glyph;
    }

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

    if ((this->laser.position >= 4 && this->laser.position < 8) &&
	this->laser.laser_state == FIRED) {
      (*value).display_glyph[this->laser.position - 4] = this->laser.glyph;
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

    // draw any lasers
    if (this->laser.position < 4 && this->laser.laser_state == FIRED) {
      (*value).display_glyph[this->laser.position] = this->laser.glyph;
    }

    return glyph_display;
  }


  (*value).display_string = "1";

  return string_display;
}




// implements f_get_display for the leds display
display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;
    
  (*value).display_int = (this->player.shield << 16) | (this->player.laser_charge << 8) | this->player.laser_value;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}



struct display_strategy* get_display_strategy(struct model *this) {
  return this->display_strategy;
}



/* player ----------------------------------------------------------- */


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


void set_player_laser_value(struct model *this, uint8_t value) {
  this->player.laser_value = value;
}

// reduce the player shield by one and return the amount of shield left
// zero is game over
int player_shield_hit(struct model *this) {
  if (this->player.shield > 0) {
    this->player.shield = this->player.shield >> 1;
    // TODO: PLAY SOUND
  }
  return this->player.shield;
}


/** set_player_as_hexdigit
 *
 * attempt to show what hex digit will be fired.  Can only show
 * if the laser has charge and the laser is not fired.  Drains
 * the laser to show this.
 * return 1 if a state change occured to drain the laser.
 */
int set_player_as_hexdigit(struct model *this) {
  int rc;
  if (this->player.laser_charge > 0) {
    this->player.glyph = invader_hex_glyphs[this->player.laser_value];
  }
  else {
      this->player.glyph = player_ship_glyph;  
  }

  rc = this->player.laser_charge_state == DRAINING ? 0 : 1; // return code is state change
  this->player.laser_charge_state = DRAINING;
  return rc;  
}

// set/revert the glyph to normal
int set_player_as_glyph(struct model *this) {
  this->player.glyph = player_ship_glyph;  
  if (this->player.laser_charge_state == DRAINING) {
    this->player.laser_charge_state = CHARGING;
    return 1; // state changed
  }
  return 0;
}


/* player laser ----------------------------------------------------------- */

void clocktick_player_laser(struct model *this) {
  if (--this->laser.laser_state_clockticks == 0 &&
      this->laser.laser_state == FIRED) {
    // do the thing and reset timer, possibly state
    this->laser.laser_state_clockticks = laser_fired_clockticks;
    if (this->laser.glyph == laser_high_glyph) {
      // we're at the high mark, move to next position
      if ((uint8_t)(this->laser.position - 4) > this->laser.position) {
	// off the display -- the laser missed
	this->laser.laser_state = READY;
      }
      else {
	// still firing through the sky
	this->laser.position = this->laser.position - 4;
	this->laser.glyph = laser_low_glyph;
      }
    }
    else {
      // same position, just move from low to high glyph
	this->laser.glyph = laser_high_glyph;
    }
  }

  if (--this->player.player_laser_state_clockticks == 0) {
    switch (this->player.laser_charge_state) {
    case CHARGING:
      this->player.laser_charge = this->player.laser_charge < 0xFF ? ((this->player.laser_charge << 1) | 0b1) : 0xFF;
      this->player.player_laser_state_clockticks = laser_charge_clockticks;
      break;
    case DRAINING:
      this->player.laser_charge = this->player.laser_charge == 0 ? 0 : this->player.laser_charge >> 1;
      this->player.player_laser_state_clockticks = laser_drain_clockticks;
      break;
    }
  }
}


/** set_player_laser_fired
 *
 * fire the laser!  the laser can only fire if fully charged.
 * It fires with the value the player set on the togles.
 * returns 1 if fired.  Zero if not.
 */
int set_player_laser_fired(struct model *this) {
  if (this->player.laser_charge == 0xFF && this->laser.laser_state == READY) {
    this->laser.value = this->player.laser_value;
    this->laser.laser_state = FIRED;
    this->laser.laser_state_clockticks = laser_fired_clockticks;
    this->laser.position = this->player.position;
    this->player.laser_charge = 0;
    this->player.glyph = laser_high_glyph;
    return 1;
  }
  return 0;
}




/* invaders ----------------------------------------------------------- */


// initialize invaders
void start_invader(struct model *this) {
  int i;

  // find next invader to start
  for (i = 0; i < num_invaders; ++i) {
    if (this->invaders[i].invader_state == INACTIVE) {
      this->invaders[i].invader_state = FORMING;
      this->invaders[i].glyph_index = 0;
      this->invaders[i].hex_value = rand() % 16;
      this->invaders[i].position = 0;
      this->invaders[i].steps = 0;
      return;
    }
  }
}


// draw the invaders.  Possibly advance them to the next square.
void clocktick_invaders(struct model *this) {
  int i;
  int move_invaders, form_invaders;
  int is_forming = 0; // do we have one forming? can only have 1 forming at a time

  if (--this->level.level_invader_speed_clock == 0) {
    // advance invaders
    this->level.level_invader_speed_clock = this->level.level_invader_speed;
    move_invaders = 1;
  }
  else {
    move_invaders = 0;
    form_invaders =
      this->level.level_invader_speed_clock < 0.5 * this->level.level_invader_speed ? 1 : 0;
  }


  for (i = 0; i < num_invaders; ++i) {
    if (this->invaders[i].invader_state == ACTIVE) {
      if (move_invaders) {
	// this should be checked-- it'll collide with player at some point
	this->invaders[i].steps++;
	this->invaders[i].position = invaders_marching_orders[this->invaders[i].steps];
      }
    }
    else if (this->invaders[i].invader_state == FORMING) {
      is_forming++;
      if (form_invaders) {
	// change state
	this->invaders[i].invader_state = ACTIVE;
	this->invaders[i].glyph = invader_hex_glyphs[this->invaders[i].hex_value];
      }
      else {
	// change glyph
	++this->invaders[i].glyph_index;
	if (this->invaders[i].glyph_index ==
	    (sizeof(invader_forming_glyphs) / sizeof(uint16_t))) {
	  this->invaders[i].glyph_index = 0;
	}
	this->invaders[i].glyph = invader_forming_glyphs[this->invaders[i].glyph_index];
      }
    }
  }

  // if it's a movement turn AND nothing is forming AND we've got
  // invaders to still introduce, make it so
  if (move_invaders == 1 && is_forming == 0 && this->level.remaining_invaders_to_form > 0) {
    this->level.remaining_invaders_to_form--;
    start_invader(this);
  }

}


void destroy_invader(struct model *this, int invader_id_collision) {
  this->invaders[invader_id_collision].invader_state = INACTIVE;
  this->invaders[invader_id_collision].position = 0;
  this->invaders[invader_id_collision].steps = 0;
}


/* levels ----------------------------------------------------------- */

const struct level get_level(struct model *this) {
  return this->level;
}

// the numbers are arbitrary...will adjust
void set_level_up(struct model *this) {
  this->level.level_number++;
  if (this->level.level_invader_speed > 10) {
      this->level.level_invader_speed--;
  }
  this->level.total_invaders += 2;
}


/* collision detection --------------------------------------------- */
int check_collision_invaders_player(struct model *this) {
  for (int i = 0; i < num_invaders; ++i) {
    if (this->invaders[i].position == this->player.position &&
	this->invaders[i].invader_state == ACTIVE) {
      // return the invader index
      return i;
    }
  }
  return -1;
}

/** check_collision_invaders_player
 *
 * return -1 for a miss
 * -2 for a hit with a bad value
 * non-negative for a legit hit
 */
int check_collision_player_laser_to_aliens(struct model *this) {
  for (int i = 0; i < num_invaders; ++i) {
    if (this->invaders[i].position == this->laser.position &&
	this->invaders[i].invader_state == ACTIVE &&
	this->laser.laser_state == FIRED) {
      this->laser.laser_state = READY;
      if (this->laser.value == this->invaders[i].hex_value) {
	// return the invader index
	return i;
      }
      else {
	return -2;
      }
    }
  }
  return -1;
}

int check_collision_invaders_laser_to_player(struct model *this) {
  return 0;
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
