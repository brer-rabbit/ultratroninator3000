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

#include "control_panel.h"
#include "game_controller.h"

struct game_controller {
  struct game_model *model;
  struct ut3k_view *view;
  char *game_exec_to_launch;
};


struct game_controller* create_game_controller(struct game_model *model, struct ut3k_view *view) {
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

void controller_update(struct game_controller *this, uint32_t clock) {
  update_view(this->view, get_display_strategy(this->model), clock);
}



/** controller callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct game_controller *this = (struct game_controller*) userdata;

  const struct button *green_button = get_green_button(control_panel);
  const struct button *blue_button = get_blue_button(control_panel);
  const struct button *red_button = get_red_button(control_panel);
  const struct rotary_encoder *rotary_encoder = get_red_rotary_encoder(control_panel);

	 
  // three finger salute: hold down green, blue, and red button last
  if (green_button->button_state == 1 &&
      blue_button->button_state == 1 &&
      red_button->button_state == 1) {
    shutdown_requested(this->model);
  }
  else if ((green_button->button_state == 0 && green_button->state_count == 0) ||
	   (blue_button->button_state == 0 && blue_button->state_count == 0)) {
    // a bit of hacky/odd behavior on this.  Probably ought to query
    // model first to see if state is in shutdown requested.  However,
    // this is safe to call even if no shutdown requested.  so...
    shutdown_aborted(this->model);
  }
    

  if (rotary_encoder->encoder_delta > 0) {
    next_game(this->model);
  }
  else if (rotary_encoder->encoder_delta < 0) {
    previous_game(this->model);
  }


  // hey, a game is picked!
  // allow either of red encoder|red button to pick executable,
  // so long as the green or blue button is NOT pressed
  if ((rotary_encoder->button.button_state == 1 &&
       rotary_encoder->button.state_count == 0) ||
      (green_button->button_state == 0 &&
       blue_button->button_state == 0 &&
       red_button->button_state == 1 &&
       red_button->state_count == 0)) {
    this->game_exec_to_launch = get_current_executable(this->model);
  }

}
