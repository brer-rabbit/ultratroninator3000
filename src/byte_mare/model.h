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

#define MAX_MOTOS_PER_GROUP 8


typedef enum { GAME_ATTRACT, GAME_PLAY_MAP, GAME_PLAY_BATTLE, GAME_OVER } game_state_t;
typedef enum { ACTIVE, DESTROYED, INACTIVE, NOT_IN_LEVEL } object_state_t;


struct ammo {
  int bullets;
  int bombs;
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
  struct ammo ammo;
  struct xy quadrant;
  struct xyz sector;
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
};


struct level {
  int num_level;
  struct moto_group moto_groups[8];
  int num_moto_groups;
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
void move_player(struct model *this, enum direction direction);




// accessors here- but these may be decoupled via the display_strategy


#endif
