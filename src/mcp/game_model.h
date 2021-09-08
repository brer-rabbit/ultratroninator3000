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

// accessible here- but may be decoupled via the display_strategy
char* get_green_string(struct game_model *this);
char* get_blue_string(struct game_model *this);
char* get_red_string(struct game_model *this);

struct display_strategy* get_display_strategy(struct game_model *this);

#endif
