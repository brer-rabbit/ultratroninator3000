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
#include "ut3k_pulseaudio.h"

typedef enum { FULL_INTRO, SHORT_INTRO, GAME_SELECT } controller_state_t;

static const int32_t state_clock_default = 25;
static void controller_update_full_intro(struct game_controller *this);


struct game_controller {
  struct game_model *model;
  struct ut3k_view *view;
  char *game_exec_to_launch;
  controller_state_t controller_state;
  uint32_t state_clock;
  int shutdown_requesting;
};



struct game_controller* create_game_controller(struct game_model *model, struct ut3k_view *view) {
  struct game_controller *this;

  this = (struct game_controller*) malloc(sizeof(struct game_controller));
  this->model = model;
  this->view = view;
  this->game_exec_to_launch = NULL;
  this->controller_state = FULL_INTRO;
  this->state_clock = state_clock_default;
  this->shutdown_requesting = 0;

  update_full_intro(this->model);
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
  switch (this->controller_state) {
  case FULL_INTRO:
    controller_update_full_intro(this);
    break;
  case SHORT_INTRO:
    //controller_update_short_intro(this);
    break;
  case GAME_SELECT:
    break;
  }

  if (clock & 0b1) {
    // keyscan only valid every 20 ms.  Assume clock is 10ms, so skip
    // every other one.
    update_controls(this->view, clock);
  }

  update_displays(this->view, get_display_strategy(this->model), clock);

  clocktick_model(this->model, clock);
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
  const struct rotary_encoder *red_rotary_encoder = get_red_rotary_encoder(control_panel);
  const struct rotary_encoder *blue_rotary_encoder = get_blue_rotary_encoder(control_panel);
	 
  // three finger salute: hold down green, blue, and red button last
  if (green_button->button_state == 1 &&
      blue_button->button_state == 1 &&
      red_button->button_state == 1) {
    shutdown_requested(this->model);
    this->shutdown_requesting = 1;
  }
  else if (this->shutdown_requesting == 1) {
    shutdown_aborted(this->model);
    this->shutdown_requesting = 0;
  }
    

  if (red_rotary_encoder->encoder_delta > 0) {
    next_game(this->model);
  }
  else if (red_rotary_encoder->encoder_delta < 0) {
    previous_game(this->model);
  }


  // hey, a game is picked!
  // allow either of red encoder|red button to pick executable,
  // so long as the green or blue button is NOT pressed
  if ((red_rotary_encoder->button.button_state == 1 &&
       red_rotary_encoder->button.state_count == 0) ||
      (red_button->button_state == 1 &&
       red_button->state_count == 0 &&
       this->shutdown_requesting == 0)) {
    this->game_exec_to_launch = get_current_executable(this->model);
  }


  if (blue_rotary_encoder->button.button_state == 1 &&
      blue_rotary_encoder->button.state_count == 0) {
    uint32_t sink_list[16];
    int num_sinks;

    ut3k_get_sink_list(sink_list, &num_sinks);
    printf("sinks: got %d\n", num_sinks);
    for (int i = 0; i< num_sinks; ++i) {
      printf("  sink %d\n", sink_list[i]);
    }
  }

}



// Static -----------------------------------------------------------------

static void controller_update_full_intro(struct game_controller *this) {
  if (--this->state_clock == 0) {
    this->state_clock = state_clock_default;
    if (update_full_intro(this->model) == 0) {
      printf("switching from full intro to short\n");
      this->controller_state = SHORT_INTRO;
      set_state_game_select(this->model);
    }
  }
}
