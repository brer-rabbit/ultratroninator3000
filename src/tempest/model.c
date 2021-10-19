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


struct flipper;
typedef enum { DISPLAY_RED, DISPLAY_BLUE, DISPLAY_GREEN } display_t;
static void display_flipper(struct flipper*, display_t, display_value*);


// abuse note: ordering is important, movement of flippers iterates the enum
// secondary abuse: 6 states for flipping transition is wildly egregious
typedef enum { INACTIVE,  // not on game board
               SPAWNING,  // on the board, viewable, can't be hit
               FLIPPING_FROM1, // leaving a space while moving, can't hit player
               FLIPPING_FROM2, // leaving a space while moving, can't hit player
               FLIPPING_FROM3, // leaving a space while moving, can't hit player
               FLIPPING_TO3, // entering a space while moving, can't hit player
               FLIPPING_TO2, // entering a space while moving, can't hit player
               FLIPPING_TO1, // entering a space while moving, can't hit player
               ACTIVE,    // moving across the board, can be hit or hit player
               DESTROYED  // blown to pieces, removed from game board
} flipper_state_t;

struct flipper {
  int position;
  int depth;
  int movement_direction; // direction the flipper takes during movement: cw (-11), ccw (1)
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

static const int default_flipper_move_timer = 50;
static const int default_flipper_flip_timer = 5;
static const int default_next_depth_timer = 250;
static const int default_spawn_next_nasty_timer = 30;
static const int default_max_spawning_nasties = 2;
static const int default_spawn_move_timer = 5;  // spawn timer + randomness
static const int default_spawn_move_timer_randomness = 5;


struct playfield {
  int num_arrays;
  int size_per_array[MAX_PLAYFIELD_NUM_ARRAYS];
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



static struct display_strategy* create_display_strategy(struct model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 
static void move_flippers(struct model *this);
static void init_playfield_for_level(struct playfield *this, int level);
static inline int index_on_depth(int from, int sizeof_from, int sizeof_to);


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
    if (this->player.position >= this->playfield.size_per_array[0] - 1) {
      this->player.position = this->playfield.circular ? 0 : this->playfield.size_per_array[0] - 1;
    }
  }
  else if (direction < 0) {
    this->player.position--;
    if (this->player.position < 0) {
      this->player.position = this->playfield.circular ?
	this->playfield.size_per_array[0] - 1 : 0;
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



/* static ----------------------------------------------------------- */


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
  struct flipper *flipper;


  for (int i = 0; i < MAX_FLIPPERS; ++i) {
    flipper = &this->playfield.flippers[i];

    switch (flipper->flipper_state) {
    case INACTIVE:
      index_inactive_flipper = i;
      break;
    case SPAWNING:
      num_flippers_spawning++; // promoted flippers will get counted here, perhaps erroneously
      break;
    case ACTIVE:

      if (flipper->depth > 0 &&
          --flipper->next_depth_timer == 0) {
        flipper->next_depth_timer = default_next_depth_timer;
        flipper->depth--;
        printf("flipper %d promoted to depth %d from position %d ", i, flipper->depth, flipper->position);
        flipper->intra_depth_timer = default_flipper_move_timer;
        // start it randomly somewhere on the next depth
        // TODO: map this to a logical position, not random
        flipper->position =
          index_on_depth(flipper->position,
                         this->playfield.size_per_array[flipper->depth + 1],
                         this->playfield.size_per_array[flipper->depth]);
          
        printf("to position %d\n", flipper->position);
      }
      else if (--flipper->intra_depth_timer == 0) {
        // reset timer to the flipper flip timer
        flipper->intra_depth_timer = default_flipper_flip_timer;
        // we're ACTIVE and our timer expired, start transition out of
        // this position
        flipper->flipper_state = FLIPPING_FROM1;

      }
      break;

    case FLIPPING_FROM3:
      // on this first clock of the state,
      // actually move to the next position, checking for boundary conditions
      if (flipper->intra_depth_timer == default_flipper_flip_timer) {
        flipper->position +=
          flipper->movement_direction;
        if (flipper->position >= this->playfield.size_per_array[flipper->depth]) {
          // don't flip past the array end
          flipper->position = 0;
        }
        else if (flipper->position < 0) {
          flipper->position =
            this->playfield.size_per_array[flipper->depth] - 1;
        }
        printf("flipper %d moved to position %d depth %d\n", i,
               flipper->position, flipper->depth);
      }

    // fallthrough
    case FLIPPING_FROM1:
    case FLIPPING_FROM2:
    case FLIPPING_TO3:
    case FLIPPING_TO2:
      if (--flipper->intra_depth_timer == 0) {
        flipper->intra_depth_timer = default_flipper_flip_timer;
        flipper->flipper_state++;
      }
      break;

    case FLIPPING_TO1:
      if (--flipper->intra_depth_timer == 0) {
        flipper->intra_depth_timer = default_flipper_move_timer;
        flipper->flipper_state = ACTIVE;
      }
      break;

    case DESTROYED:
    default:
      break;
    } // switch

  } // for
}





static void init_playfield_for_level(struct playfield *playfield, int level) {

  if (1 || level == 1) {
    *playfield = (struct playfield const)
      {
       .num_arrays = 5,
       .size_per_array = { 28, 20, 12, 4, 4 },
       .circular = 1,
       .spawn_next_nasty_timer = default_spawn_next_nasty_timer,
       .num_flippers = 8,
      };

    // zero out array
    memset(playfield->arrays, 0, sizeof(int) * MAX_PLAYFIELD_NUM_ARRAYS * MAX_PLAYFIELD_ARRAY_SIZE);

    // TODO: delete / refactor code once spawning rules are created
    // for now, manually spawn two flippers

    for (int i = 2; i < playfield->num_flippers; ++i) {
      // put flippers at max depth, inactive, and randomly positioned
      playfield->flippers[i].depth = playfield->num_arrays - 1;
      playfield->flippers[i].flipper_state = INACTIVE;
      playfield->flippers[i].position = rand() %
        playfield->size_per_array[ playfield->num_arrays - 1 ];
    }

    playfield->flippers[0].depth = playfield->num_arrays - 1;
    playfield->flippers[0].flipper_state = ACTIVE;
    playfield->flippers[0].position = rand() %
      playfield->size_per_array[ playfield->num_arrays - 1 ];
    playfield->flippers[0].intra_depth_timer =
      default_spawn_move_timer + rand() % default_spawn_move_timer_randomness;
    playfield->flippers[0].next_depth_timer = default_next_depth_timer;
    playfield->flippers[0].movement_direction = rand() & 0b1 ? 1 : -1;
    
    playfield->flippers[1].depth = playfield->num_arrays - 1;
    playfield->flippers[1].flipper_state = ACTIVE;
    playfield->flippers[1].position = rand() %
      playfield->size_per_array[ playfield->num_arrays - 1 ];
    playfield->flippers[1].intra_depth_timer =
      default_spawn_move_timer + rand() % default_spawn_move_timer_randomness;
    playfield->flippers[1].next_depth_timer = default_next_depth_timer;
    playfield->flippers[1].movement_direction = rand() & 0b1 ? 1 : -1;

  }
}


static inline int index_on_depth(int from, int sizeof_from, int sizeof_to) {
  return from * sizeof_to / sizeof_from;
}



/* view methods ----------------------------------------------------- */


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

  // draw any active flippers
  for (int i = 0; i < MAX_FLIPPERS; ++i) {
    display_flipper(&this->playfield.flippers[i], DISPLAY_RED, value);
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

  // draw any active flippers
  for (int i = 0; i < MAX_FLIPPERS; ++i) {
    display_flipper(&this->playfield.flippers[i], DISPLAY_BLUE, value);
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

  // draw any active flippers
  for (int i = 0; i < MAX_FLIPPERS; ++i) {
    display_flipper(&this->playfield.flippers[i], DISPLAY_GREEN, value);
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




struct flipper_position_to_display_digit_and_glyph {
  display_t display;
  int digit;
  uint16_t glyph_active;
  uint16_t glyph_move[3];
};

// this completely convoluted data structure starts at the center of
// the playfield, each list starts at 9 o'clock and goes CCW around
// from there.

// lookup by flipper position for depth3 ("d3")
static const struct flipper_position_to_display_digit_and_glyph flipper_glyph_by_position_d4[] =
  {
   { DISPLAY_BLUE, 1, 0x0004, { 0x0004, 0x0004, 0x0004 } },
   { DISPLAY_BLUE, 2, 0x0010, { 0x0010, 0x0010, 0x0010 } },
   { DISPLAY_BLUE, 2, 0x0020, { 0x0020, 0x0020, 0x0020 } },
   { DISPLAY_BLUE, 1, 0x0002, { 0x0002, 0x0002, 0x0002 } }
  };


static const struct flipper_position_to_display_digit_and_glyph flipper_glyph_by_position_d3[] =
  {
   { DISPLAY_BLUE, 1, 0x1000, { 0x2000, 0x0080, 0x0004 } },
   { DISPLAY_BLUE, 2, 0x1000, { 0x0800, 0x0040, 0x0010 } },
   { DISPLAY_BLUE, 2, 0x0200, { 0x0100, 0x0040, 0x0020 } },
   { DISPLAY_BLUE, 1, 0x0200, { 0x0400, 0x0080, 0x0002 } }
  };

static const struct flipper_position_to_display_digit_and_glyph flipper_glyph_by_position_d2[] =
  // 12 elements
  {
   { DISPLAY_BLUE,  1, 0x0010, { 0x0008, 0x0800, 0x2000 } },
   { DISPLAY_RED,   1, 0x0020, { 0x0001, 0x0100, 0x0040 } },
   { DISPLAY_RED,   1, 0x0002, { 0x0001, 0x0200, 0x0400 } },
   { DISPLAY_RED,   2, 0x0020, { 0x0001, 0x0100, 0x0040 } },
   { DISPLAY_RED,   2, 0x0002, { 0x0001, 0x0200, 0x0400 } },
   { DISPLAY_BLUE,  2, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_BLUE,  2, 0x0002, { 0x0400, 0x0200, 0x0100 } },
   { DISPLAY_GREEN, 2, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_GREEN, 2, 0x0008, { 0x1000, 0x0800, 0x0040 } },
   { DISPLAY_GREEN, 1, 0x2000, { 0x0004, 0x0080, 0x0004 } },
   { DISPLAY_GREEN, 1, 0x0008, { 0x1000, 0x0800, 0x0040 } },
   { DISPLAY_BLUE,  1, 0x0020, { 0x0001, 0x0100, 0x0400 } }
  };


static const struct flipper_position_to_display_digit_and_glyph flipper_glyph_by_position_d1[] =
  // 20 elements
  {
   { DISPLAY_BLUE,  0, 0x0004, { 0x0040, 0x2000, 0x1000 } },
   { DISPLAY_RED,   0, 0x0001, { 0x0200, 0x0400, 0x0002 } },
   { DISPLAY_RED,   0, 0x0002, { 0x0080, 0x0400, 0x0200 } },
   { DISPLAY_RED,   1, 0x0020, { 0x0100, 0x0200, 0x0040 } },
   { DISPLAY_RED,   1, 0x0001, { 0x0200, 0x0400, 0x0080 } },
   { DISPLAY_RED,   2, 0x0020, { 0x0100, 0x0200, 0x0040 } },
   { DISPLAY_RED,   2, 0x0001, { 0x0200, 0x0400, 0x0080 } },
   { DISPLAY_RED,   3, 0x0020, { 0x0040, 0x0100, 0x0200 } },
   { DISPLAY_RED,   3, 0x0001, { 0x0200, 0x0100, 0x0040 } },
   { DISPLAY_BLUE,  3, 0x0010, { 0x0800, 0x1000, 0x2000 } },
   { DISPLAY_BLUE,  3, 0x0020, { 0x0100, 0x0200, 0x0400 } },
   { DISPLAY_GREEN, 3, 0x0008, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_GREEN, 3, 0x0010, { 0x0040, 0x1000, 0x0800 } },
   { DISPLAY_GREEN, 2, 0x0004, { 0x1000, 0x2000, 0x1000 } },
   { DISPLAY_GREEN, 2, 0x0008, { 0x0010, 0x0800, 0x1000 } },
   { DISPLAY_GREEN, 1, 0x0004, { 0x1000, 0x2000, 0x1000 } },
   { DISPLAY_GREEN, 1, 0x0008, { 0x0010, 0x0800, 0x1000 } },
   { DISPLAY_GREEN, 0, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_GREEN, 0, 0x0008, { 0x0800, 0x1000, 0x2000 } },
   { DISPLAY_BLUE,  0, 0x0002, { 0x0400, 0x0200, 0x0100 } }
  };


static const struct flipper_position_to_display_digit_and_glyph flipper_glyph_by_position_d0[] =
  // 28 elements
  {
   { DISPLAY_BLUE,  0, 0x0010, { 0x0800, 0x1000, 0x2000 } },
   { DISPLAY_RED,   0, 0x0020, { 0x0100, 0x0200, 0x0400 } },
   { DISPLAY_RED,   0, 0x0010, { 0x0040, 0x0800, 0x1000 } },
   { DISPLAY_RED,   0, 0x0008, { 0x0800, 0x0040, 0x0080 } },
   { DISPLAY_RED,   0, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_RED,   1, 0x0008, { 0x0800, 0x0040, 0x0080 } },
   { DISPLAY_RED,   1, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_RED,   2, 0x0008, { 0x0800, 0x0040, 0x0080 } },
   { DISPLAY_RED,   2, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_RED,   3, 0x0008, { 0x0800, 0x0040, 0x0080 } },
   { DISPLAY_RED,   3, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_RED,   3, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_RED,   3, 0x0002, { 0x0400, 0x0200, 0x0100 } },
   { DISPLAY_BLUE,  3, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_BLUE,  3, 0x0002, { 0x0400, 0x0200, 0x0100 } },
   { DISPLAY_GREEN, 3, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_GREEN, 3, 0x0002, { 0x0400, 0x0200, 0x0100 } },
   { DISPLAY_GREEN, 3, 0x0400, { 0x0001, 0x0200, 0x0080 } },
   { DISPLAY_GREEN, 3, 0x0100, { 0x0001, 0x0200, 0x0040 } },
   { DISPLAY_GREEN, 2, 0x0400, { 0x0001, 0x0200, 0x0080 } },
   { DISPLAY_GREEN, 2, 0x0100, { 0x0001, 0x0200, 0x0040 } },
   { DISPLAY_GREEN, 1, 0x0400, { 0x0001, 0x0200, 0x0080 } },
   { DISPLAY_GREEN, 1, 0x0100, { 0x0001, 0x0200, 0x0040 } },
   { DISPLAY_GREEN, 0, 0x0400, { 0x0001, 0x0200, 0x0080 } },
   { DISPLAY_GREEN, 0, 0x0100, { 0x0001, 0x0200, 0x0040 } },
   { DISPLAY_GREEN, 0, 0x0020, { 0x0100, 0x0200, 0x0400 } },
   { DISPLAY_GREEN, 0, 0x0010, { 0x0800, 0x1000, 0x2000 } },
   { DISPLAY_BLUE,  0, 0x0020, { 0x0100, 0x0200, 0x0400 } }
  };



static const struct flipper_position_to_display_digit_and_glyph *flipper_glyph_by_depth_position[] =
  {
   flipper_glyph_by_position_d0,
   flipper_glyph_by_position_d1,
   flipper_glyph_by_position_d2,
   flipper_glyph_by_position_d3,
   flipper_glyph_by_position_d4
  };


static void display_flipper(struct flipper *flipper, display_t display, display_value *value) {
  int flipper_depth = flipper->depth;
  int flipper_position = flipper->position;
  int flipper_state = flipper->flipper_state;

    
  if (flipper_glyph_by_depth_position[flipper_depth][flipper_position].display == display) {
    // trying for at least a little readability from nested lookups...

    switch (flipper_state) {
    case ACTIVE:      
      (*value).display_glyph[ flipper_glyph_by_depth_position[flipper_depth][flipper_position].digit ] |=
        flipper_glyph_by_depth_position[flipper_depth][flipper_position].glyph_active;
      break;
    case FLIPPING_FROM1:
    case FLIPPING_FROM2:
    case FLIPPING_FROM3:
      (*value).display_glyph[ flipper_glyph_by_depth_position[flipper_depth][flipper_position].digit ] |=
        flipper_glyph_by_depth_position[flipper_depth][flipper_position].glyph_move[flipper_state - FLIPPING_FROM1];
      break;
    case FLIPPING_TO3:
    case FLIPPING_TO2:
    case FLIPPING_TO1:
      // reverse index the glyph_move array: start high, go low
      (*value).display_glyph[ flipper_glyph_by_depth_position[flipper_depth][flipper_position].digit ] |=
        flipper_glyph_by_depth_position[flipper_depth][flipper_position].glyph_move[-1 * (flipper_state - FLIPPING_TO1)];

    default:
      break;
    }
  }

}
