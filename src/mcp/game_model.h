/* game_model.h
 *
 * define the model
 */

#ifndef GAME_MODEL_H
#define GAME_MODEL_H

#include "display_strategy.h"

struct game_model;

/* init model.  Caller is responsible for allocating game_model memory */
struct game_model* create_game_model();
void free_game_model(struct game_model*);

void next_game(struct game_model *this);
void previous_game(struct game_model *this);
char* get_current_executable(struct game_model *this);

// accessible here- but may be decoupled via the display_strategy
char* get_green_string(struct game_model *this);
char* get_blue_string(struct game_model *this);
char* get_red_string(struct game_model *this);

struct display_strategy* get_display_strategy(struct game_model *this);

#endif
