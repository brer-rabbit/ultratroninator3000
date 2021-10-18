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





// view
static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);


typedef enum { INACTIVE, SPAWNING, ACTIVE, DESTROYED } flipper_state_t;

struct flipper {
  int position;
  int depth;
  int movement_direction; // direction the flipper takes during ascending
  flipper_state_t flipper_state;
  int intra_depth_timer; // timer for moving within the current depth
  int next_depth_timer; // timer for advancing to next depth (well, depth -1, surfacing?)
};


  
struct player {
  int position;
  int depth; // thinking this will always be zero, but include here anyway...
};


// the playfield is a list of arrays...
// level zero being where the player is.  The greatest number (num_levels-1)
// is where the flippers originate.
#define MAX_PLAYFIELD_NUM_ARRAYS 5
#define MAX_PLAYFIELD_ARRAY_SIZE 28
#define MAX_FLIPPERS 16

static const int default_flipper_move_timer = 10;
static const int default_next_depth_timer = 300;
static const int default_spawn_next_nasty_timer = 30;
static const int default_max_spawning_nasties = 2;
static const int default_spawn_movement_timer = 5;  // spawn timer + randomness
static const int default_spawn_movement_timer_randomness = 5;


struct playfield {
  int num_arrays;
  int max_index_by_array[MAX_PLAYFIELD_NUM_ARRAYS];
  int arrays[MAX_PLAYFIELD_NUM_ARRAYS][MAX_PLAYFIELD_ARRAY_SIZE];
  int circular; // boolean- should it wrap around?  applies to all arrays
  int spawn_next_nasty_timer; // timer to spawn next enemy
  int num_flippers;
  struct flipper flippers[MAX_FLIPPERS];
};

struct model {
  struct display_strategy *display_strategy;
  struct player player;
  struct playfield playfield;
};



static void init_playfield_for_level(struct playfield *this, int level);
static struct display_strategy* create_display_strategy(struct model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 
static void move_flippers(struct model *this);



/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));
  this->display_strategy = create_display_strategy(this);

  this->player = (struct player const) { 0 };

  // just for now...get things going
  init_playfield_for_level(&this->playfield, 1);

  return this;
}


void free_model(struct model *this) {
  free_display_strategy(this->display_strategy);
  return free(this);
}


void clocktick_gameplay(struct model *this, uint32_t clock) {
  // move the nasties, decrement clock, make adjustments
  move_flippers(this);
}


void move_player(struct model *this, int8_t direction) {

  if (direction > 0) {
    this->player.position++;
    if (this->player.position >= this->playfield.max_index_by_array[0]) {
      this->player.position = this->playfield.circular ? 0 : this->playfield.max_index_by_array[0] - 1;
    }
  }
  else if (direction < 0) {
    this->player.position--;
    if (this->player.position < 0) {
      this->player.position = this->playfield.circular ?
	this->playfield.max_index_by_array[0] - 1 : 0;
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


/** move_flippers
 *
 * 1 move any active flippers
 * 2 move spawning flippers in the pool, advance to active if clockticks down
 * 3 promote inactive to spawning if there is room in the pool
 */
static void move_flippers(struct model *this) {
  int num_flippers_spawning = 0;
  int index_inactive_flipper = -1; // pick one, any one inactive

  for (int i = 0; i < MAX_FLIPPERS; ++i) {
    switch (this->playfield.flippers[i].flipper_state) {
    case INACTIVE:
      index_inactive_flipper = i;
      break;
    case SPAWNING:
      num_flippers_spawning++; // promoted flippers erroneously counted, it's fine.
      // fallthrough...
    case ACTIVE:

      if (this->playfield.flippers[i].depth > 0 &&
          --this->playfield.flippers[i].next_depth_timer == 0) {
        this->playfield.flippers[i].next_depth_timer = default_next_depth_timer;
        this->playfield.flippers[i].flipper_state = ACTIVE;
        this->playfield.flippers[i].depth--;
        printf("flipper %d promoted to depth %d\n", i, this->playfield.flippers[i].depth);
        this->playfield.flippers[i].intra_depth_timer = default_flipper_move_timer;
        // start it randomly somewhere on the next depth
        // TODO: map this to a logical position
        this->playfield.flippers[i].position = rand() % this->playfield.max_index_by_array[this->playfield.flippers[i].depth];
      }
      else if (--this->playfield.flippers[i].intra_depth_timer == 0) {
        // reset timer, move flipper, check boundary of current depth
        this->playfield.flippers[i].intra_depth_timer = default_flipper_move_timer;
        this->playfield.flippers[i].position +=
          this->playfield.flippers[i].movement_direction;
        if (this->playfield.flippers[i].position > this->playfield.max_index_by_array[this->playfield.flippers[i].depth]) {
          // don't flip past the array end
          this->playfield.flippers[i].position = 0;
        }
        else if (this->playfield.flippers[i].position < 0) {
          this->playfield.flippers[i].position =
            this->playfield.max_index_by_array[this->playfield.flippers[i].depth];
        }
        printf("flipper %d moved to position %d depth %d\n", i,
               this->playfield.flippers[i].position, this->playfield.flippers[i].depth);
      }

    } // switch

  } // for
}


/* static ----------------------------------------------------------- */


static void init_playfield_for_level(struct playfield *playfield, int level) {

  if (1 || level == 1) {
    *playfield = (struct playfield const)
      {
       .num_arrays = 4,
       .max_index_by_array = { 27, 19, 11, 9, 0 },
       .circular = 1,
       .spawn_next_nasty_timer = default_spawn_next_nasty_timer,
       .num_flippers = 8,
      };

    // zero out array
    memset(playfield->arrays, 0, sizeof(int) * MAX_PLAYFIELD_NUM_ARRAYS * MAX_PLAYFIELD_ARRAY_SIZE);

    for (int i = 2; i < playfield->num_flippers; ++i) {
      // put flippers at max depth, inactive, and randomly positioned
      playfield->flippers[i].depth = playfield->num_arrays - 1;
      playfield->flippers[i].flipper_state = INACTIVE;
      playfield->flippers[i].position = rand() %
        playfield->max_index_by_array[ playfield->num_arrays - 1 ];
    }

    playfield->flippers[0].depth = playfield->num_arrays - 1;
    playfield->flippers[0].flipper_state = SPAWNING;
    playfield->flippers[0].position = rand() %
      playfield->max_index_by_array[ playfield->num_arrays - 1 ];
    playfield->flippers[0].intra_depth_timer =
      default_spawn_movement_timer + rand() % default_spawn_movement_timer_randomness;
    playfield->flippers[0].next_depth_timer = default_next_depth_timer;
    playfield->flippers[0].movement_direction = rand() & 0b1 ? 1 : -1;
    
    playfield->flippers[1].depth = playfield->num_arrays - 1;
    playfield->flippers[1].flipper_state = SPAWNING;
    playfield->flippers[1].position = rand() %
      playfield->max_index_by_array[ playfield->num_arrays - 1 ];
    playfield->flippers[1].intra_depth_timer =
      default_spawn_movement_timer + rand() % default_spawn_movement_timer_randomness;
    playfield->flippers[1].next_depth_timer = default_next_depth_timer;
    playfield->flippers[1].movement_direction = rand() & 0b1 ? 1 : -1;

  }
}



/* view methods */


static uint16_t player_glyph_red_blue[] =
   { 0x0160, 0x0850, 0x1800, 0x3000, 0x1800, 0x3000, 0x1800, 0x3000,
     0x1800, 0x3000, 0x2084, 0x0482 };

static uint16_t player_glyph_green[] =
  { 0x2084, 0x0482, 0x0600, 0x0300,  0x0600, 0x0300,  0x0600, 0x0300,
    0x0600, 0x0300, 0x0160, 0x0850 };

struct flipper_position_to_display_digit_and_glyph {
  int display;
  int digit;
  uint16_t glyph;
};

static const struct flipper_position_to_display_digit_and_glyph flipper_glyph_by_depth3[] =
  {
   { 1, 1, 0x2000 },
   { 1, 1, 0x0004,},
   { 1, 2, 0x0010 },
   { 1, 2, 0x0800 },
   { 1, 2, 0x0040 },
   { 1, 2, 0x0100 },
   { 1, 2, 0x0020 },
   { 1, 1, 0x0002 },
   { 1, 1, 0x0400 },
   { 1, 1, 0x0080 }
  };


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

  // draw any spawning/active flippers
  for (int i = 0; i < MAX_FLIPPERS; ++i) {
    if ((this->playfield.flippers[i].flipper_state == SPAWNING ||
        this->playfield.flippers[i].flipper_state == ACTIVE) &&
        this->playfield.flippers[i].depth == 3) {
      
      (*value).display_glyph[ flipper_glyph_by_depth3[ this->playfield.flippers[i].position ].digit ] |=
        flipper_glyph_by_depth3[ this->playfield.flippers[i].position ].glyph;
    }
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


