/* Copyright 2021 Kyle Farrell
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


/* game_controller.h
 *
 */

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

struct game_controller;

#include "game_model.h"
#include "ut3k_view.h"
#include "control_panel.h"


struct game_controller* create_game_controller(struct game_model*, struct ut3k_view*);
void free_game_controller(struct game_controller *this);

/** get_game_to_launch
 * if a game was set, this will return it.
 * poll on this function to get it.
 */
char* get_game_to_launch(struct game_controller *this);

void controller_update(struct game_controller *this, uint32_t clock);

void controller_callback_control_panel(struct control_panel *control_panel, void *userdata);

#endif
