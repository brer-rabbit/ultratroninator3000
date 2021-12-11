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

#define MAX_MOTO_GROUPS 8
#define MAX_MOTOS_PER_GROUP 8
#define MAX_FLIGHT_PATH_LENGTH 512
#define TERRAIN_DISTANCE 6


typedef enum { GAME_ATTRACT, GAME_PLAY_MAP, GAME_PLAY_FLIGHT_TUNNEL, GAME_PLAY_BATTLE, GAME_OVER } game_state_t;
typedef enum { ACTIVE, DESTROYED, INACTIVE, NOT_IN_LEVEL } object_state_t;


struct stores {
  int bullets;
  int fuel;
  int shields;
};

struct xy {
  int x;
  int y;
};

struct xyz {
  int x;
  int y;
  int z;
};

struct player_bullet {
  object_state_t status;
  struct xyz position;
  uint8_t timer;
};

struct player {
  struct stores stores;
  struct xy quadrant;
  struct xy cursor_quadrant;
  struct xyz sector;
  struct xy flight_tunnel;
};

struct moto {
  struct xyz sector;
  object_state_t status;
};

struct moto_group {
  struct xy quadrant;
  object_state_t status; // is it visible yet?
  int num_motos;
  struct moto motos[MAX_MOTOS_PER_GROUP];
  int movement_timer_reset;
  int movement_timer_remaining;
};


// flight corridor: this is a single y point in the path of flight.
// It has a "width" of an opening the player must fly through.  The
// offset is how far from the left wall the opening is.
struct flight_path_slice {
  int width;
  int offset;
};

// flight path is a sequence of flight_path_slices the player must
// navigate through.  The sequence scrolls by one when the timer ticks
// down.
struct flight_path {
  struct flight_path_slice slice[MAX_FLIGHT_PATH_LENGTH];
  int flight_path_length;
  int scroll_timer_reset;
  int scroll_timer_remaining;
};

struct level {
  int num_level;
  struct moto_group moto_groups[MAX_MOTO_GROUPS];
  int num_moto_groups;
};

struct battle {
  struct moto_group *moto_group; // pointer to which group is in battle
  int terrain_map[TERRAIN_DISTANCE];
  // values for determining next terrain step:
  int terrain_segment_length;
  int terrain_distance_on_segment; // increment til terrain_distance_to_next_pitch_change
  int terrain_start_elevation;
  int terrain_end_elevation;
  int terrain_scroll_timer_reset;
  int terrain_scroll_timer_remaining;
};


struct model;

/* create a model for the caller.  Mem is allocated in the function;
 * caller is responsible for freeing later via free_game_mode
 */
struct model* create_model();
void free_model(struct model*);



// Supported model methods
game_state_t clocktick_model(struct model *this);

game_state_t get_game_state(struct model *this);
void set_game_start(struct model *this);

const struct player* get_player(struct model *this);

void map_move_cursor(struct model *this, enum direction direction);
void set_game_state_flight_tunnel(struct model *this);

const struct moto_group* get_moto_groups(struct model *this);

const struct flight_path* get_flight_path(struct model *this);
int is_flight_path_complete(struct model *this);
void flight_move_player(struct model *this, int position);


void set_game_state_battle(struct model *this);
const struct battle* get_battle(struct model *this);
void battle_move_player_x(struct model *this, int amount);
void battle_move_player_y(struct model *this, int amount);
void battle_move_player_z(struct model *this, int amount);


#endif
