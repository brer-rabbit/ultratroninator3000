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

#include "tempest.h"
#include "model.h"
#include "ut3k_pulseaudio.h"


typedef enum
{
  GAME_ATTRACT,
  GAME_PLAY,
  GAME_PLAYER_HIT,
  GAME_LEVEL_UP,
  GAME_OVER
} game_state_t;
  

// I keep calling this the model, but it's got the view coupled...
static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);


// the playfield is a list of arrays...
// level zero being where the player is.  The greatest number (num_levels-1)
// is where the flippers originate.
#define MAX_PLAYFIELD_NUM_ARRAYS 5
#define MAX_PLAYFIELD_ARRAY_SIZE 28
#define MAX_FLIPPERS 16
#define MAX_BLASTER_SHOTS 2



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
  int movement_direction; // direction the flipper takes during movement: cw (-11), none (0), ccw (1)
  flipper_state_t flipper_state;
  int intra_depth_timer; // timer for moving within the current depth
  int next_depth_timer; // timer for advancing to next depth (well, depth -1, surfacing?)
};

typedef enum { READY, FIRED } blaster_state_t;
struct blaster {
  int position;
  int depth;
  blaster_state_t blaster_state;
  int move_timer;
};
  
struct player {
  int position;
  int depth; // thinking this will always be zero, but include here anyway...
  struct blaster blaster[MAX_BLASTER_SHOTS];
  int blaster_ready_timer; // non-positive is a ready state
};



static const int default_flipper_move_timer = 25;
static const int default_flipper_flip_timer = 5;
static const int default_next_depth_timer = 99;
static const int default_player_blaster_move_timer = 6;
static const int default_player_blaster_ready_timer = 60;
static const int default_next_flipper_spawn_timer_randmoness = 200;
static const int default_next_flipper_spawn_timer_minumum = 50;
static const int default_max_spawning_flippers = 3;

struct playfield {
  int num_arrays;
  int size_per_array[MAX_PLAYFIELD_NUM_ARRAYS];
  int arrays[MAX_PLAYFIELD_NUM_ARRAYS][MAX_PLAYFIELD_ARRAY_SIZE];
  int circular; // boolean- should it wrap around?  applies to all arrays
  int num_flippers;
  int next_flipper_spawn_timer;
  struct flipper flippers[MAX_FLIPPERS];
};


struct model {
  struct display_strategy *gameplay_display_strategy;
  struct player player;
  struct playfield playfield;
  game_state_t game_state;
};


// view methods
typedef enum { DISPLAY_RED, DISPLAY_BLUE, DISPLAY_GREEN } display_t;
static void display_flipper(struct flipper*, display_t, display_value*);
static void display_blaster(struct blaster*, display_t, display_value*);

// model methods
static struct display_strategy* create_gameplay_display_strategy(struct model *this);
static void free_gameplay_display_strategy(struct display_strategy *display_strategy); 
static void clocktick_gameplay(struct model *this, uint32_t clock);
static int move_flippers(struct model *this);
static int move_blasters(struct model *this);
static void collision_check(struct model *this);
static void spawn_flipper(struct model *this, int index);
static void init_playfield_for_level(struct playfield *this, int level);
static inline int index_on_depth(int from, int sizeof_from, int sizeof_to);


/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));
  this->gameplay_display_strategy = create_gameplay_display_strategy(this);

  this->player = (struct player const) { 0 };

  // just for now...get things going
  this->game_state = GAME_PLAY;
  init_playfield_for_level(&this->playfield, 1);

  return this;
}


void free_model(struct model *this) {
  free_gameplay_display_strategy(this->gameplay_display_strategy);
  return free(this);
}


void clocktick_model(struct model *this, uint32_t clock) {
  switch(this->game_state) {
  case GAME_PLAY:
    clocktick_gameplay(this, clock);
    break;
  case GAME_PLAYER_HIT:
    break;
  default:
    break;
  }
}


static void clocktick_gameplay(struct model *this, uint32_t clock) {
  // move the nasties, decrement clock, make adjustments
  move_flippers(this);
  move_blasters(this);
  collision_check(this);
  // unchecked decrement blaster timer.
  // Non-positive is ready this will roll/error from negative to
  // positive in ~1 year.  That's acceptable.
  this->player.blaster_ready_timer--;
}


/** move_player
 *
 * player circles the arena at depth zero.  If the playfield allows wrap,
 * do that.  Otherwise held at the array boundaries.
 */
void move_player(struct model *this, int8_t direction) {

  if (direction > 0) {
    this->player.position++;
    if (this->player.position >= this->playfield.size_per_array[0]) {
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
  ut3k_play_sample(player_move_soundkey);
}


/** set_player_blaster_fired
 *
 * fire the thingymajig.  Conditions are the blaster timer is non-positive
 * and shots are available.  Firing set a blaster shot to fired at player
 * position and reset of the blaster timer.
 */
void set_player_blaster_fired(struct model *this) {
  if (this->player.blaster_ready_timer <= 0) {
    // timer says it's available, see if any shots are ready
    struct blaster *blaster;
    for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
      blaster = &this->player.blaster[i];
      if (blaster->blaster_state == READY) {
        // got one
        *blaster = (struct blaster const)
          {
           .blaster_state = FIRED,
           .position = this->player.position,
           .depth = this->player.depth,
           .move_timer = default_player_blaster_move_timer
          };

        if (i == 0) {
          ut3k_play_sample(player_multishot_soundkey);
        }
        else {
          ut3k_play_sample(player_shoot_soundkey);
        }
        this->player.blaster_ready_timer = default_player_blaster_ready_timer;
        return;
      }
    }
  }
}




struct display_strategy* get_display_strategy(struct model *this) {
  return this->gameplay_display_strategy;
}






struct display_strategy* create_gameplay_display_strategy(struct model *this) {
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


static void free_gameplay_display_strategy(struct display_strategy *display_strategy) {
  free(display_strategy);
}


/** move_flippers
 *
 * 1 move any active flippers
 * 2 move spawning flippers in the pool, advance to active if clockticks down
 * 3 promote inactive to spawning if there is room in the pool
 * return true if something moved that requires collision detection,
 * zero for nothing changed that would require collision detection.
 */
static int move_flippers(struct model *this) {
  int something_moved = 0;
  int num_flippers_spawning = 0;
  int index_inactive_flipper = -1; // pick one, any one inactive
  struct flipper *flipper;


  for (int i = 0; i < this->playfield.num_flippers; ++i) {
    flipper = &this->playfield.flippers[i];

    switch (flipper->flipper_state) {
    case INACTIVE:
      index_inactive_flipper = i;
      break;
    case SPAWNING:
      num_flippers_spawning++;
      if (--flipper->next_depth_timer == 0) {
        something_moved = 1;
        flipper->flipper_state = ACTIVE;
        flipper->depth = this->playfield.num_arrays - 1;
        flipper->position = rand() %
          this->playfield.size_per_array[ this->playfield.num_arrays - 1 ];
        flipper->next_depth_timer = default_next_depth_timer;
        flipper->intra_depth_timer = default_flipper_move_timer;
      }
      else {
        // make it move around a bit- then as the next depth timer
        // runs down launch from position 0-->7
        flipper->position = -1 * ( 8 * flipper->next_depth_timer / default_next_depth_timer - 7);
      }
      break;
    case ACTIVE:

      if (flipper->depth > 0 &&
          --flipper->next_depth_timer == 0) {
        something_moved = 1;
        flipper->next_depth_timer = default_next_depth_timer;
        flipper->depth--;
        //printf("flipper %d promoted to depth %d from position %d ", i, flipper->depth, flipper->position);
        flipper->intra_depth_timer = default_flipper_move_timer;

        flipper->position =
          index_on_depth(flipper->position,
                         this->playfield.size_per_array[flipper->depth + 1],
                         this->playfield.size_per_array[flipper->depth]);
        //printf("to position %d\n", flipper->position);
          
      }
      else if (--flipper->intra_depth_timer == 0) {
        // reset timer to the flipper flip timer
        something_moved = 1;
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
        something_moved = 1;
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
        //printf("flipper %d moved to position %d depth %d\n", i, flipper->position, flipper->depth);
      }

    // fallthrough
    case FLIPPING_FROM1:
    case FLIPPING_FROM2:
    case FLIPPING_TO3:
    case FLIPPING_TO2:
      if (--flipper->intra_depth_timer == 0) {
        something_moved = 1;
        flipper->intra_depth_timer = default_flipper_flip_timer;
        flipper->flipper_state++;
      }
      break;

    case FLIPPING_TO1:
      if (--flipper->intra_depth_timer == 0) {
        something_moved = 1;
        flipper->intra_depth_timer = default_flipper_move_timer;
        flipper->flipper_state = ACTIVE;
      }
      break;

    case DESTROYED:
    default:
      break;
    } // switch

  } // for

  // if nothing is spawning, setup an inactive flipper
  if (--this->playfield.next_flipper_spawn_timer == 0) {
    this->playfield.next_flipper_spawn_timer = rand() % default_next_flipper_spawn_timer_randmoness + default_next_flipper_spawn_timer_minumum;
    if (num_flippers_spawning < default_max_spawning_flippers &&
        index_inactive_flipper != -1) {
      //printf("spawning flipper index %d\n", index_inactive_flipper);
      spawn_flipper(this, index_inactive_flipper);
    }
  }

  return something_moved;
}


/** move_blasters
 *
 * move the blaster shots according to their timer.  Return true
 * if something actually moved; zero for nothing changed.
 */
static int move_blasters(struct model *this) {
  int something_moved = 0;
  struct blaster *blaster;

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    blaster = &this->player.blaster[i];
    if (blaster->blaster_state == FIRED) {
      if (--blaster->move_timer == 0) {
        something_moved = 1;
        blaster->move_timer = default_player_blaster_move_timer;
        if (blaster->depth == this->playfield.num_arrays - 1) {
          // blaster shot is at the max depth, reset back to ready
          blaster->blaster_state = READY;
        }
        else {
          // move the shot
          printf("blaster %d moved from %d depth %d ",
                 i, blaster->position, blaster->depth);
          blaster->position =
            index_on_depth(blaster->position,
                           this->playfield.size_per_array[blaster->depth],
                           this->playfield.size_per_array[blaster->depth + 1]);
          blaster->depth++;
          printf("to %d depth %d\n",
                 blaster->position, blaster->depth);

        }
      }
    }
  }
  return something_moved;
}


/** collision_check
 *
 * pre: everything has moved for this clocktick.
 * check if any blaster shots occupy the position of a nasty.
 * Check if any nasty is in the same space as the player.
 */
static void collision_check(struct model *this) {
  struct flipper *flipper;
  struct blaster *blaster;

  // plain vanilla O(N*N) approach to this
  for (int i = 0; i < MAX_FLIPPERS; ++i) {
    flipper = &this->playfield.flippers[i];
    if (flipper->flipper_state == INACTIVE ||
        flipper->flipper_state == SPAWNING ||
        flipper->flipper_state == DESTROYED) {
      continue; // ignore this flipper
    }

    // blaster can destroy an active or flipping nasty
    for (int j = 0; j < MAX_BLASTER_SHOTS; ++j) {
      blaster = &this->player.blaster[j];
      if (blaster->blaster_state == READY) {
        continue;
      }
      else if (blaster->depth == flipper->depth &&
               blaster->position == flipper->position) {
        // HIT!
        ut3k_play_sample(enemy_destroyed_soundkey);
        blaster->blaster_state = READY;
        flipper->flipper_state = DESTROYED;
      }
    }

    // any flipper active now has a chance to hit the player
    // flippers flipping between spaces don't hit the player
    if (flipper->flipper_state == ACTIVE &&
        flipper->depth == this->player.depth &&
        flipper->position == this->player.position) {
      // buh bye Player 1
      printf("Player hit!\n");
      // TODO: game state transiton, play sound
    }
  }
}



/** spawn_flipper
 *
 * bring an INACTIVE flipper to life.
 * Set a random time for to go from spawning to active.
 */
static void spawn_flipper(struct model *this, int index) {
  struct flipper *flipper = &this->playfield.flippers[index];

  *flipper = (struct flipper const)
    {
     .flipper_state = SPAWNING,
     .position = 0,
     .next_depth_timer = default_next_depth_timer,
     .intra_depth_timer = 0,
     .movement_direction = rand() & 0b1 ? 1 : -1
    };
}



static void init_playfield_for_level(struct playfield *playfield, int level) {

  if (1 || level == 1) {
    *playfield = (struct playfield const)
      {
       .num_arrays = sizeof((((struct playfield *)0)->size_per_array)) / sizeof(int*),
       .size_per_array = { 28, 20, 12, 4, 4 },
       //.size_per_array = { 14, 10, 6, 4, 4 },
       .circular = 1,
       .num_flippers = 12,
       .next_flipper_spawn_timer = rand() % default_next_flipper_spawn_timer_randmoness + default_next_flipper_spawn_timer_minumum
      };

    // zero out array
    memset(playfield->arrays, 0, sizeof(int) * MAX_PLAYFIELD_NUM_ARRAYS * MAX_PLAYFIELD_ARRAY_SIZE);


    for (int i = 0; i < playfield->num_flippers; ++i) {
      // put flippers at max depth, inactive, and randomly positioned
      playfield->flippers[i].depth = playfield->num_arrays - 1;
      playfield->flippers[i].flipper_state = INACTIVE;
    }
  }
}


static inline int index_on_depth(int from, int sizeof_from, int sizeof_to) {
  return from * sizeof_to / sizeof_from;
}



/* view methods ----------------------------------------------------- */


static const uint16_t player_glyph[] =
  {
   0x0850, 0x0160, 0x0850, 0x1800, 0x3000, 0x1800, 0x3000, 0x1800, 0x3000,
   0x1800, 0x3000, 0x2084, 0x0482, 0x2084, 0x0482, 0x2084, 0x0482, 0x0600,
   0x0300, 0x0600, 0x0300, 0x0600, 0x0300, 0x0600, 0x0300, 0x0160, 0x0850,
   0x0160
  };


// implements f_get_display for the red display
static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  *brightness = HT16K33_BRIGHTNESS_12;
  *blink = HT16K33_BLINK_OFF;

  memset((*value).display_glyph, 0, sizeof(display_value));

  if (this->player.position != 0) {
    if (this->player.position > 0 && this->player.position < 5) {
      (*value).display_glyph[0] = player_glyph[this->player.position];
    }
    else if (this->player.position < 7) {
      (*value).display_glyph[1] = player_glyph[this->player.position];
    }  
    else if (this->player.position < 9) {
      (*value).display_glyph[2] = player_glyph[this->player.position];
    }  
    else if (this->player.position < 13) {
      (*value).display_glyph[3] = player_glyph[this->player.position];
    }  
  }

  // draw any active flippers
  for (int i = 0; i < this->playfield.num_flippers; ++i) {
    display_flipper(&this->playfield.flippers[i], DISPLAY_RED, value);
  }

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    struct blaster *blaster = &this->player.blaster[i];
    if (blaster->blaster_state == FIRED) {
      display_blaster(blaster, DISPLAY_RED, value);
    }
  }

  return glyph_display;
}

// implements f_get_display for the blue display
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  memset((*value).display_glyph, 0, sizeof(display_value));

  if (this->player.position == 0 || this->player.position == 27) {
    (*value).display_glyph[0] = player_glyph[this->player.position];
  }
  else if (this->player.position == 13 || this->player.position == 14) {
    (*value).display_glyph[3] = player_glyph[this->player.position];
  }

  // draw any active flippers
  for (int i = 0; i < this->playfield.num_flippers; ++i) {
    display_flipper(&this->playfield.flippers[i], DISPLAY_BLUE, value);
  }

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    struct blaster *blaster = &this->player.blaster[i];
    if (blaster->blaster_state == FIRED) {
      display_blaster(blaster, DISPLAY_BLUE, value);
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

  if (this->player.position != 27) {
    if (this->player.position > 22) {
      (*value).display_glyph[0] = player_glyph[this->player.position];
    }
    else if (this->player.position > 20) {
      (*value).display_glyph[1] = player_glyph[this->player.position];
    }
    else if (this->player.position > 18) {
      (*value).display_glyph[2] = player_glyph[this->player.position];
    }
    else if (this->player.position > 14) {
      (*value).display_glyph[3] = player_glyph[this->player.position];
    }
  }

  // draw any active flippers
  for (int i = 0; i < this->playfield.num_flippers; ++i) {
    display_flipper(&this->playfield.flippers[i], DISPLAY_GREEN, value);
  }

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    struct blaster *blaster = &this->player.blaster[i];
    if (blaster->blaster_state == FIRED) {
      display_blaster(blaster, DISPLAY_GREEN, value);
    }
  }

  return glyph_display;
}




// implements f_get_display for the leds display
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  (*value).display_int = 0;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  (*value).display_int = 0;

  for (int i = 0; i < this->playfield.num_flippers; ++i) {
    if (this->playfield.flippers[i].flipper_state == SPAWNING) {
      (*value).display_int |= ((1 << this->playfield.flippers[i].position) << 8);
    }
  }


  return integer_display;
}





// this completely convoluted data structure starts at the center of
// the playfield, each list starts at 9 o'clock and goes CCW around
// from there.

// this is intended to handle blaster, flippers, and maybe other stuff
// that gets created...and it's a mess.
struct object_position_to_display_digit_and_glyph {
  display_t display;
  int digit;
  uint16_t glyph_active;
  uint16_t glyph_move[3];
};

static const struct object_position_to_display_digit_and_glyph blaster_glyph_by_position_d1[] =
  {
   { DISPLAY_BLUE, 0, 0x4000, { } },
   { DISPLAY_RED,  0, 0x0400, { } },
   { DISPLAY_RED,  0, 0x0400, { } },
   { DISPLAY_RED,  1, 0x0020, { } },
   { DISPLAY_RED,  1, 0x0002, { } },
   { DISPLAY_RED,  2, 0x0200, { } },
   { DISPLAY_RED,  3, 0x0020, { } },
   { DISPLAY_RED,  3, 0x0100, { } },
   { DISPLAY_RED,  3, 0x0001, { } },
   { DISPLAY_BLUE, 3, 0x0008, { } },
   { DISPLAY_BLUE, 3, 0x0001, { } },
   { DISPLAY_GREEN,3, 0x0800, { } },
   { DISPLAY_GREEN,3, 0x0010, { } },
   { DISPLAY_GREEN,2, 0x0004, { } },
   { DISPLAY_GREEN,2, 0x0010, { } },
   { DISPLAY_GREEN,1, 0x1000, { } },
   { DISPLAY_GREEN,0, 0x0004, { } },
   { DISPLAY_GREEN,0, 0x2000, { } },
   { DISPLAY_GREEN,0, 0x0008, { } },
   { DISPLAY_BLUE, 0, 0x0080, { } }
  };

static const struct object_position_to_display_digit_and_glyph blaster_glyph_by_position_d3[] =
  {
   { DISPLAY_BLUE, 1, 0x0004, { } },
   { DISPLAY_BLUE, 2, 0x0010, { } },
   { DISPLAY_BLUE, 2, 0x0020, { } },
   { DISPLAY_BLUE, 1, 0x0002, { } }
  };

static const struct object_position_to_display_digit_and_glyph *blaster_glyph_by_depth_position[] =
  {
   NULL,
   blaster_glyph_by_position_d1,
   NULL,
   blaster_glyph_by_position_d3,
   NULL
  };



// lookup by flipper position for depth3 ("d3")
static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d4[] =
  {
   { DISPLAY_BLUE, 1, 0x0004, { 0x0004, 0x0004, 0x0004 } },
   { DISPLAY_BLUE, 2, 0x0010, { 0x0010, 0x0010, 0x0010 } },
   { DISPLAY_BLUE, 2, 0x0020, { 0x0020, 0x0020, 0x0020 } },
   { DISPLAY_BLUE, 1, 0x0002, { 0x0002, 0x0002, 0x0002 } }
  };


static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d3[] =
  {
   { DISPLAY_BLUE, 1, 0x1000, { 0x2000, 0x0080, 0x0004 } },
   { DISPLAY_BLUE, 2, 0x1000, { 0x0800, 0x0040, 0x0010 } },
   { DISPLAY_BLUE, 2, 0x0200, { 0x0100, 0x0040, 0x0020 } },
   { DISPLAY_BLUE, 1, 0x0200, { 0x0400, 0x0080, 0x0002 } }
  };

static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d2[] =
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


static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d1[] =
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


static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d0[] =
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



static const struct object_position_to_display_digit_and_glyph *flipper_glyph_by_depth_position[] =
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


static void display_blaster(struct blaster *blaster, display_t display, display_value *value) {
  int blaster_depth = blaster->depth;
  int blaster_position = blaster->position;

    
  if (blaster_glyph_by_depth_position[blaster->depth] != NULL) {
    if (blaster_glyph_by_depth_position[blaster->depth][blaster_position].display == display) {
      (*value).display_glyph[ blaster_glyph_by_depth_position[blaster_depth][blaster_position].digit ] |=
        blaster_glyph_by_depth_position[blaster_depth][blaster_position].glyph_active;
    }
  }

}
