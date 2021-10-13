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




/* game_model.h
 *
 * define the model
 */

#ifndef GAME_MODEL_H
#define GAME_MODEL_H

#include "display_strategy.h"

struct game_model;

/* create a model for the caller.  Mem is allocated in the function;
 * caller is responsible for freeing later via free_game_mode
 */
struct game_model* create_game_model();
void free_game_model(struct game_model*);


// Supported model methods

void next_game(struct game_model *this);
void previous_game(struct game_model *this);
char* get_current_executable(struct game_model *this);

void set_blink(struct game_model *this, int on);


void shutdown_requested(struct game_model *this);
void shutdown_aborted(struct game_model *this);



struct display_strategy* get_display_strategy(struct game_model *this);

#endif
