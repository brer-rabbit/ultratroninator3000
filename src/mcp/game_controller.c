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

#include <stdlib.h>
#include <stdio.h>

#include "game_controller.h"

struct game_controller {
  struct game_model *model;
  struct game_view *view;
  char *game_exec_to_launch;
};


struct game_controller* create_game_controller(struct game_model *model, struct game_view *view) {
  struct game_controller *this;

  this = (struct game_controller*) malloc(sizeof(struct game_controller));
  this->model = model;
  this->view = view;
  this->game_exec_to_launch = NULL;
  return this;
}

void free_game_controller(struct game_controller *this) {
  free(this);
}

char* get_game_to_launch(struct game_controller *this) {
  // get the game to launch- if it returns non-NULL, prep for launch!
  char *game_to_launch = this->game_exec_to_launch;

  return game_to_launch;
}

void controller_update(struct game_controller *this) {
  update_view(this->view, get_display_strategy(this->model));
}

void controller_callback_green_rotary_encoder(int8_t delta, uint8_t button_pushed, uint8_t button_changed, void *userdata) {
  struct game_controller *this = (struct game_controller*) userdata;

  if (button_pushed) { // launch game!
    this->game_exec_to_launch = get_current_executable(this->model);
    set_blink(this->model, 1);
    return; // don't process any delta from the rotary encoder
  }

  if (delta > 0) {
    next_game(this->model);
  }
  else if (delta < 0) {
    previous_game(this->model);
  }


}
