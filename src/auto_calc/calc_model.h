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




/* calc_model.h
 *
 * define the model
 */

#ifndef CALC_MODEL_H
#define CALC_MODEL_H

#include "display_strategy.h"

struct calc_model;

/* create a model for the caller.  Mem is allocated in the function;
 * caller is responsible for freeing later via free_game_mode
 */
struct calc_model* create_calc_model();
void free_calc_model(struct calc_model*);

// Supported model methods


void update_green_register(struct calc_model *this);
void update_blue_register(struct calc_model *this);
void update_red_register(struct calc_model *this);


// accessible here- but may be decoupled via the display_strategy
void set_green_register(struct calc_model *this, uint8_t value);
int32_t get_green_register(struct calc_model *this);
int32_t get_blue_register(struct calc_model *this);
int32_t get_red_register(struct calc_model *this);

// swap the two input registers
void swap_registers(struct calc_model *this);


// f_calc declarations
// int32_t is used to detect overflow to make it interesting
typedef int32_t (*f_calc)(uint8_t, uint8_t);
int32_t f_calc_add(uint8_t a, uint8_t b);
int32_t f_calc_sub(uint8_t a, uint8_t b);
int32_t f_calc_or(uint8_t a, uint8_t b);
int32_t f_calc_and(uint8_t a, uint8_t b);
void set_calc_function(struct calc_model *this, f_calc f_calculator);



// f_next_value declarations
typedef uint8_t (*f_next_value)(uint8_t);
uint8_t f_next_plus_one(uint8_t a);
uint8_t f_next_minus_one(uint8_t a);
uint8_t f_next_times_two(uint8_t a);
uint8_t f_next_random(uint8_t a);
void set_next_value_blue_function(struct calc_model *, f_next_value);


struct display_strategy* get_display_strategy(struct calc_model *this);

#endif
