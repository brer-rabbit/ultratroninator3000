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
struct model* create_model(char ***sample_keys);
void free_model(struct model*);

// Supported model methods

void set_next_step(struct model*, uint8_t step);
void clocktick_model(struct model*);
void toggle_run_state(struct model *this);
void set_bpm(struct model *this, int bpm);
int get_bpm(struct model *this);
void change_bpm(struct model *this, int amount); // change by a relative amount

void change_instrument(struct model *this, int amount);
void change_instrument_sample(struct model *this, int sample_num);

void toggle_current_triggered_instrument_at_step(struct model *this, int step);
void set_trigger_at_step(struct model *this, int instrument, int step);
void clear_trigger_at_step(struct model *this, int instrument, int step);

typedef enum {
  SHUFFLE_OFF = 0,
  SHUFFLE_LOW = 1,
  SHUFFLE_MEDIUM = 2,
  SHUFFLE_HIGH = 3
} shuffle_t;

void set_shuffle(struct model *this, shuffle_t shuffle);


struct display_strategy* get_display_strategy(struct model *this);

#endif
