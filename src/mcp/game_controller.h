/* game_controller.h
 *
 */

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "game_model.h"
#include "game_view.h"

struct game_controller;


struct game_controller* create_game_controller(struct game_model *model, struct game_view *view);
void controller_update_data(struct game_controller *this, char *redstr);

#endif
