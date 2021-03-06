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
#include "controller.h"
#include "ut3k_pulseaudio.h"


struct controller {
  struct model *model;
  struct ut3k_view *view;

};


/* start off with accurate front panel state */
static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel);



struct controller* create_controller(struct model *model, struct ut3k_view *view) {
  struct controller *this;

  this = (struct controller*) malloc(sizeof(struct controller));
  this->model = model;
  this->view = view;

  initialize_model_from_control_panel(this, get_control_panel(view));

  return this;
}

void free_controller(struct controller *this) {
  free(this);
}


void controller_update(struct controller *this, uint32_t clock) {
  if (clock & 0b1) {
    update_controls(this->view, clock);
  }

  update_displays(this->view, get_display_strategy(this->model), clock);

  clocktick_model(this->model, clock);
}


static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel) {
}


/** controller callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct controller *this = (struct controller*) userdata;
  const struct rotary_encoder *red_rotary_encoder = get_red_rotary_encoder(control_panel);
  const struct button *blue_button = get_blue_button(control_panel);
  const struct toggles *toggles = get_toggles(control_panel);


  if (red_rotary_encoder->encoder_delta != 0) {
    move_player(this->model, -1 * red_rotary_encoder->encoder_delta);
  }


  if (blue_button->button_state == 1) {
    set_player_blaster_fired(this->model);
  }


  if (toggles->state_count == 0 && toggles->toggles_toggled == 0x01) {
    set_player_zapper(this->model);
  }
}
