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

/* create a model for the caller.  Mem is allocated in the function;
 * caller is responsible for freeing later via free_game_mode
 */
struct model* create_model();
void free_model(struct model*);

// Supported model methods

// high level state changes
typedef enum { GAME_ATTRACT, GAME_OVER, GAME_PLAYING, GAME_LEVEL_UP } game_state_t;
game_state_t get_game_state(struct model*);
void game_start(struct model*);
void game_over(struct model*);
void game_attract(struct model*);


void player_left(struct model*);
void player_right(struct model*);
void set_player_laser_value(struct model *this, uint8_t value);
int player_shield_hit(struct model *this);
int set_player_as_hexdigit(struct model*);
int set_player_as_glyph(struct model*);

void clocktick_player_laser(struct model*);
int set_player_laser_fired(struct model*);

void start_invader(struct model*);
void clocktick_invaders(struct model*);
void destroy_invader(struct model*, int invader_id_collision);

const struct level get_level(struct model *this);
void set_level_up(struct model *this);


int check_collision_invaders_player(struct model*);
int check_collision_player_laser_to_aliens(struct model*);
int check_collision_invaders_laser_to_player(struct model*);

void game_over_scroll(struct model *this);

// critical to implement
struct display_strategy* get_display_strategy(struct model *this);

#endif
