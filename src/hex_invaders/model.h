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
typedef enum { GAME_OVER, GAME_PLAYING } game_state_t;
typedef enum { READY, FIRED, CHARGING } laser_state_t;
typedef enum { FORMING, ACTIVE, INACTIVE } invader_state_t;

void game_start(struct model*);

void player_left(struct model*);
void player_right(struct model*);
void start_invader(struct model*);
void update_invaders(struct model*);

// accessors here- but these may be decoupled via the display_strategy


// critical to implement
struct display_strategy* get_display_strategy(struct model *this);

#endif
