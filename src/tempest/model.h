/* 
 * Copyright 2021 Kyle Farrell
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




/* model.h
 *
 * define the model
 */

#ifndef MODEL_H
#define MODEL_H

#include "display_strategy.h"

struct model;


// the playfield is a list of arrays...
// level zero being where the player is.  The greatest number (num_levels-1)
// is where the flippers originate.
#define MAX_PLAYFIELD_NUM_ARRAYS 5
#define MAX_PLAYFIELD_ARRAY_SIZE 28
#define MAX_FLIPPERS 16
#define MAX_BLASTER_SHOTS 2

// data structs
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
  int lives_remaining;
};

struct playfield {
  int num_arrays;
  int size_per_array[MAX_PLAYFIELD_NUM_ARRAYS];
  int arrays[MAX_PLAYFIELD_NUM_ARRAYS][MAX_PLAYFIELD_ARRAY_SIZE];
  int circular; // boolean- should it wrap around?  applies to all arrays
  int num_flippers;
  int next_flipper_spawn_timer;
  struct flipper flippers[MAX_FLIPPERS];
  int has_collision;
};


typedef enum { FLASH_SCREEN, LIVES_REMAINING, RESTART } player_hit_state_t;
struct player_hit_and_restart {
  int timer;
  player_hit_state_t state;
  int scroll_timer;
  char messaging[48];
  char *msg_ptr;
};


/* create a model for the caller.  Mem is allocated in the function;
 * caller is responsible for freeing later via free_game_mode
 */
struct model* create_model();
void free_model(struct model*);

// Supported model methods

void clocktick_model(struct model*, uint32_t clock);

// move: positive/negative amount to move player by
void move_player(struct model*, int8_t direction);
void set_player_blaster_fired(struct model*);


// get the view
struct display_strategy* get_display_strategy(struct model *this);

const struct player* get_model_player(struct model *this);
const struct playfield* get_model_playfield(struct model *this);
const struct player_hit_and_restart* get_model_player_hit_and_restart(struct model *this);


#endif
