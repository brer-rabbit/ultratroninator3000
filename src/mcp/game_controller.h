/* game_controller.h
 *
 */

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

struct game_controller;

#include "game_model.h"
#include "game_view.h"


struct game_controller* create_game_controller(struct game_model*, struct game_view*);
void free_game_controller(struct game_controller *this);

void controller_update(struct game_controller *this);

void controller_callback_green_rotary_encoder(int8_t delta, uint8_t button_pushed, uint8_t button_changed, void *userdata);

#endif
