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

#include "hex_inv_ader.h"
#include "model.h"
#include "ut3k_pulseaudio.h"


typedef enum { FIRED, READY } laser_state_t;
typedef enum { CHARGING, DRAINING } player_laser_state_t;
typedef enum { FORMING, ACTIVE, INACTIVE, DESTROYED } invader_state_t;



// "position":
// number the characters of the displays, three 4-character displays, 0-11:
//
//  0  1  2  3  (green)
//  4  5  6  7  (blue)
//  8  9 10 11  (red)
//
// swap direction on each level
static uint8_t invaders_marching_orders1[] = { 0, 1, 2, 3, 7, 6, 5, 4, 8, 9, 10, 11 };
static uint8_t invaders_marching_orders2[] = { 3, 2, 1, 0, 4, 5, 6, 7, 11, 10, 9, 8 };

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
  uint8_t invader_speed;
  uint8_t invader_speed_clockticks_til_move;
  uint8_t invaders_to_destroy;
  uint8_t remaining_invaders_to_form;
  uint8_t *invaders_marching_orders;
  uint8_t invaders_level_bitmask;
};

// what to display during game over
#define MESSAGING_MAX_LENGTH 32
struct messaging {
  // have longer messages, and point to which character in message to display
  char green_display_message[MESSAGING_MAX_LENGTH];
  char *green_display;
  char blue_display_message[MESSAGING_MAX_LENGTH];
  char *blue_display;
  char red_display_message[MESSAGING_MAX_LENGTH];
  char *red_display;
  int scroll_index;
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
  struct messaging messaging;
};



static void set_level_up(struct model *this);
static struct display_strategy* create_display_strategy(struct model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 

static uint16_t player_ship_glyph = 0b0010100000001000;

static uint16_t laser_high_glyph = 0b0000001000000000;
static uint16_t laser_low_glyph = 0b0001000000000000;

const static uint16_t invader_forming_glyphs[] =
  { 0x0040, 0x0100, 0x0200, 0x0400, 0x0080, 0x2000, 0x1000, 0x0800 };
   
const static uint8_t level_one_invader_glyph_index = 16;
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
   0b0000000001110001,  // F
   0b0010110111100010  // X(sort of) -- level 1 invader
  };


/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));
  this->display_strategy = create_display_strategy(this);

  game_attract(this);
  //game_start(this);

  return this;
}


game_state_t get_game_state(struct model *this) {
  return this->game_state;
}


void game_start(struct model *this) {
  this->level.level_number = 1;
  this->level.invader_speed = 255;  // clockticks decs to zero then reset to this
  this->level.invader_speed_clockticks_til_move = this->level.invader_speed;
  this->level.remaining_invaders_to_form = 8;
  this->level.invaders_to_destroy = this->level.remaining_invaders_to_form;
  this->level.invaders_marching_orders = invaders_marching_orders1;
  this->level.invaders_level_bitmask = 0;
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


void game_level_up(struct model *this) {
  if (this->game_state != GAME_PLAYING ||
      this->level.invaders_to_destroy != 0) {
    return;
  }

  
  set_level_up(this);
  snprintf(this->messaging.green_display_message, MESSAGING_MAX_LENGTH,
	   "   LEVEL %-2d COMPLETE  ", this->level.level_number - 1);
  this->messaging.green_display = this->messaging.green_display_message;

  // increase shield by 1
  if (this->player.shield != 0xFF) {
    this->player.shield = this->player.shield << 1 | 0b1;
  }
  if (this->player.shield == 0xFF) {
    snprintf(this->messaging.blue_display_message, MESSAGING_MAX_LENGTH,
	   "    SHIELD MAXIMUM   ");
  }
  else {
    snprintf(this->messaging.blue_display_message, MESSAGING_MAX_LENGTH,
	   "    SHIELD INCREASED   ");
  }
  this->messaging.blue_display = this->messaging.blue_display_message;


  this->game_state = GAME_LEVEL_UP;
}


// level_up scroller
void level_up_scroll(struct model *this) {
  this->messaging.green_display =
    (this->messaging.green_display[4] == '\0') ?
    this->messaging.green_display_message :
    this->messaging.green_display + 1;

  this->messaging.blue_display =
    (this->messaging.blue_display[4] == '\0') ?
    this->messaging.blue_display_message :
    this->messaging.blue_display + 1;

  // pointer arithmetic made this less useful than I first
  // thought it'd be.  Can still use this for the LED lights
  // to flash the increase in shield
  if (this->messaging.scroll_index < MESSAGING_MAX_LENGTH) {
    ++this->messaging.scroll_index;
  }
  else {
    this->messaging.scroll_index = 0;
  }


}




// continue from level_up state
void game_resume(struct model *this) {
  this->game_state = GAME_PLAYING;
}

void game_over(struct model *this) {
    
  // hack of a message..get it to display over two cycles
  snprintf(this->messaging.green_display_message, MESSAGING_MAX_LENGTH,
	   "GAMEGAMEOVEROVER");
  // magic number...since it flip flops first time in, start with "OVER"
  this->messaging.green_display = &(this->messaging.green_display_message[12]);
  snprintf(this->messaging.blue_display_message, MESSAGING_MAX_LENGTH,
	   "    SCORE %-4d   ", this->player.score);
  this->messaging.blue_display = this->messaging.blue_display_message;
  snprintf(this->messaging.red_display_message, MESSAGING_MAX_LENGTH,
	   "      RANK SPACE CADET   ");
  this->messaging.red_display = this->messaging.red_display_message;
  this->game_state = GAME_OVER;
}



// scrolling specific the game_over
void game_over_scroll(struct model *this) {

  // green game over just flip flops between "GAME" and "OVER"...
  // pointer arithmetic with strings, this isn't a good idea...
  if (this->messaging.green_display - 12 == this->messaging.green_display_message) {
    this->messaging.green_display = this->messaging.green_display_message;
  }
  else {
    this->messaging.green_display = this->messaging.green_display + 4;
  }

  this->messaging.blue_display =
    (this->messaging.blue_display[4] == '\0') ?
    this->messaging.blue_display_message :
    this->messaging.blue_display + 1;

  this->messaging.red_display =
    (this->messaging.red_display[4] == '\0') ?
    this->messaging.red_display_message :
    this->messaging.red_display + 1;

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

  if (this->game_state == GAME_PLAYING ||
      this->game_state == GAME_LEVEL_UP) {
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
      if ((this->invaders[i].invader_state == ACTIVE ||
	   this->invaders[i].invader_state == FORMING) &&
	  this->invaders[i].position >= 8) {
	(*value).display_glyph[this->invaders[i].position - 8] =
	  this->invaders[i].glyph;
      }
    }

    return glyph_display;
  }
  else if (this->game_state == GAME_OVER) {
    (*value).display_string = this->messaging.red_display;
    return string_display;
  }
  else {
    (*value).display_string = "NOOP";
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
      if ((this->invaders[i].invader_state == ACTIVE ||
	   this->invaders[i].invader_state == FORMING) &&
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
  else if (this->game_state == GAME_OVER ||
	   this->game_state == GAME_LEVEL_UP) {
    (*value).display_string = this->messaging.blue_display;
    return string_display;
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
      if ((this->invaders[i].invader_state == ACTIVE ||
	   this->invaders[i].invader_state == FORMING) &&
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
  else if (this->game_state == GAME_OVER ||
	   this->game_state == GAME_LEVEL_UP) {
    (*value).display_string = this->messaging.green_display;
    return string_display;
  }


  (*value).display_string = "1";

  return string_display;
}




// implements f_get_display for the leds display
display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;
  uint8_t shield = this->player.shield;
  // flash the increase in shield if we're levelling up and increasing shield
  if (this->game_state == GAME_LEVEL_UP && shield != 0xFF && this->messaging.scroll_index % 2) {
    shield = shield >> 1;
  }

  (*value).display_int = (shield << 16) | (this->player.laser_charge << 8) | this->player.laser_value;
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
  if (this->game_state == GAME_PLAYING || this->game_state == GAME_LEVEL_UP) {
    if (this->player.position > 8) {
      this->player.position--;
    }
  }
}

void player_right(struct model *this) {
  if (this->game_state == GAME_PLAYING || this->game_state == GAME_LEVEL_UP) {
    if (this->player.position < 11) {
      this->player.position++;
    }
  }
}


void set_player_laser_value(struct model *this, uint8_t value) {
  this->player.laser_value = value;
  ut3k_play_sample(laser_toggled_soundkey);
}

// reduce the player shield by one and return the amount of shield left
// zero is game over
int player_shield_hit(struct model *this) {
  if (this->player.shield > 0) {
    this->player.shield = this->player.shield >> 1;
  }
  return this->player.shield;
}


/** set_player_as_hexdigit
 *
 * attempt to show what hex digit will be fired.  Can only show
 * if the laser has charge.  Slowly drains the laser to show this.
 * return 1 if a state change occured to drain the laser.
 */
int set_player_as_hexdigit(struct model *this) {
  int rc;
  if (this->game_state != GAME_PLAYING) {
    return 0;
  }

  if (this->player.laser_charge > 0) {
    this->player.glyph = invader_hex_glyphs[this->player.laser_value];
  }
  else {
      this->player.glyph = player_ship_glyph;  
  }

  rc = this->player.laser_charge_state == DRAINING ? 0 : 1; // return code is state change
  if (rc) { // only play sample if state changed
    ut3k_play_sample(showhex_soundkey);
  }

  this->player.laser_charge_state = DRAINING;
  return rc;  
}

// set/revert the glyph to normal
int set_player_as_glyph(struct model *this) {
  this->player.glyph = player_ship_glyph;  
  if (this->player.laser_charge_state == DRAINING) {
    this->player.laser_charge_state = CHARGING;
    ut3k_play_sample(hidehex_soundkey);
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
    ut3k_play_sample(playerfire_soundkey);
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
      // hex value is dependent on game level.  Higher levels get more bits.
      // index 16
      this->invaders[i].hex_value =
	(this->level.level_number == 1) ?
	level_one_invader_glyph_index :
	(rand() % 16) & this->level.invaders_level_bitmask;
      this->invaders[i].position = this->level.invaders_marching_orders[0];
      this->invaders[i].steps = 0;
      return;
    }
  }
}


// draw the invaders.  Possibly advance them to the next square.
void clocktick_invaders(struct model *this) {
  int i;
  int move_invaders, form_invaders = 0, active_invaders = 0;
  int is_forming = 0; // do we have one forming? can only have 1 forming at a time

  if (--this->level.invader_speed_clockticks_til_move == 0) {
    // advance invaders
    this->level.invader_speed_clockticks_til_move = this->level.invader_speed;
    move_invaders = 1;
  }
  else {
    move_invaders = 0;
    form_invaders =
      this->level.invader_speed_clockticks_til_move < 0.5 * this->level.invader_speed ? 1 : 0;
  }


  for (i = 0; i < num_invaders; ++i) {
    if (this->invaders[i].invader_state == ACTIVE) {
      active_invaders++;
      if (move_invaders) {
	// advance invaders to next step
	// probably ought to assert it's
	this->invaders[i].steps++;
	this->invaders[i].position = this->level.invaders_marching_orders[this->invaders[i].steps];
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
	// change glyph as it forms
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
  // invaders to still introduce, so form one up
  if (move_invaders == 1) {
    if (is_forming == 0 && this->level.remaining_invaders_to_form > 0) {
      this->level.remaining_invaders_to_form--;
      start_invader(this);
      ut3k_play_sample(invader_forming_soundkey);
    }
    else {
      if (active_invaders) {
	ut3k_play_sample(moveinvaders1_soundkey);
      }
    }
  }

}


void destroy_invader(struct model *this, int invader_id_collision) {
  this->invaders[invader_id_collision].invader_state = DESTROYED;
  this->invaders[invader_id_collision].position = 0;
  this->invaders[invader_id_collision].steps = 0;
  this->level.invaders_to_destroy--;
}

int get_invaders_remaining(struct model *this) {
  return this->level.invaders_to_destroy;
}



/* levels ----------------------------------------------------------- */

const struct level get_level(struct model *this) {
  return this->level;
}

// set the data for the next level
static void set_level_up(struct model *this) {
  if (this->level.invader_speed > 10) {
    this->level.invader_speed = this->level.invader_speed - 20;
  }

  // level 1 starts with no mask, then add a bit on each level to finally end at 0xF
  if (this->level.level_number == 1) {
    this->level.invaders_level_bitmask = 1;
  }
  else if  (this->level.level_number < 5) {
    this->level.invaders_level_bitmask = (this->level.invaders_level_bitmask << 1) | 0b1;
  }
    
  // switch directions-- point to the opposite table as previous
  this->level.invaders_marching_orders =
    this->level.invaders_marching_orders == invaders_marching_orders1 ?
    invaders_marching_orders2 : invaders_marching_orders1;

  this->level.remaining_invaders_to_form = 8;
  this->level.invaders_to_destroy = this->level.remaining_invaders_to_form;
  for (int i = 0; i < num_invaders; ++i) {
    this->invaders[i].invader_state = INACTIVE;
    this->invaders[i].position = 0;
    this->invaders[i].steps = 0;
  }

  this->level.level_number++;
}


/* collision detection --------------------------------------------- */

/** check_collision_invaders_player
 *
 * check if the invader and player collided.  Return -1 for no collision,
 * or an index/handle for the invader that collided.
 */
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
      // level 1 requires no hex value
      if (this->laser.value == this->invaders[i].hex_value ||
	  this->level.level_number == 1) {
	// return the invader index
	// player score + 1
	this->player.score++;
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
  // we don't have the invaders firing lasers...
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
