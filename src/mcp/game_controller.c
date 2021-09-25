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
static int count = 0;
void controller_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct game_controller *this = (struct game_controller*) userdata;

  const struct button *button = get_blue_button(control_panel);
  const struct rotary_encoder *rotary_encoder = get_red_rotary_encoder(control_panel);
  const struct selector *selector = get_green_selector(control_panel);
  const struct toggles *toggles = get_toggles(control_panel);

  if (button->state_count == 0) {
    printf("blue button changed from %d to %d after %d cycles\n",
	   button->button_previous_state,
	   button->button_state,
	   button->button_previous_state_count);
  }
	 
  if (rotary_encoder->encoder_delta > 0) {
    printf("encoder state 0x%X delta %d (%d ticks to neutral, table %p) count %d\n", rotary_encoder->encoder_state, rotary_encoder->encoder_delta, rotary_encoder->clock_ticks_to_neutral, (void*)rotary_encoder->encoder_lookup_table, count);
    next_game(this->model);
  }
  else if (rotary_encoder->encoder_delta < 0) {
    previous_game(this->model);
  }


  if (rotary_encoder->button.state_count == 0) {
    printf("rotary encoder button changed from %d to %d after %d cycles\n",
	   rotary_encoder->button.button_previous_state,
	   rotary_encoder->button.button_state,
	   rotary_encoder->button.button_previous_state_count);
  }


  if (selector->state_count == 0) {
    printf("selector changed from %d to %d count %d bits 0x%X\n", selector->selector_previous_state, selector->selector_state, selector->state_count, selector->selector_state_bits);
  }


  if (toggles->state_count == 0) {
    printf("toggles changed from 0x%X to 0x%X (change: 0x%X)\n",
	   toggles->toggles_previous_state,
	   toggles->toggles_state,
	   toggles->toggles_toggled);
  }


}
