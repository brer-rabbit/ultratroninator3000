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

typedef enum { GAME_ATTRACT, GAME_SERVE, GAME_PLAY, GAME_OVER } game_state_t;
typedef enum { HANDICAP_EXPERT = 0, HANDICAP_MED2 = 1, HANDICAP_MED1 = 2, HANDICAP_NOVICE = 3 } handicap_t;

/* create a model for the caller.  Mem is allocated in the function;
 * caller is responsible for freeing later via free_game_mode
 */

struct model* create_model();
void free_model(struct model*);
game_state_t clocktick_model(struct model*);



struct player {
  handicap_t handicap; // length beyond one unit
  int y_position;
  int score;
  int velocity;
};

struct ball {
  int x_position;
  int y_position;
  int x_clocks_per_move; // velocity
  int x_move_timer;
  int y_clocks_per_move; // velocity
  int y_move_timer;
  int x_direction;
  int y_direction;
};
  


// Supported model methods
game_state_t get_game_state(struct model *this);


void player1_move(struct model *this, int distance);
void player2_move(struct model *this, int distance);
void player1_button_pushed(struct model *this);
void player2_button_pushed(struct model *this);

void set_player1_handicap(struct model *this, handicap_t handicap);
void set_player2_handicap(struct model *this, handicap_t handicap);

const struct player* get_player1(struct model *this);
const struct player* get_player2(struct model *this);

const struct ball* get_ball(struct model *this);

// accessors here- but these may be decoupled via the display_strategy


#endif
