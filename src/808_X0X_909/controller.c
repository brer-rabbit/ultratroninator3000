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


/** controller_update:
 * do the thing and also return the current BPM to the caller.
 * This is expected to be called once every 128th note.
 */
int controller_update(struct controller *this, uint32_t clock) {
  clocktick_model(this->model);

  if (clock & 0b1) {
    update_controls(this->view, clock);
  }

  update_displays(this->view, get_display_strategy(this->model), clock);

  return get_bpm(this->model);
}


static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel) {
}


// toggles are high bit on left with low on right, so swap bit
// ordering when mapping to steps
static const uint8_t steps_to_toggle_map[] = { 7, 6, 5, 4, 3, 2, 1, 0 };

/** controller callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct controller *this = (struct controller*) userdata;
  const struct rotary_encoder *green_rotary = get_green_rotary_encoder(control_panel);
  const struct selector *green_selector = get_green_selector(control_panel);

  const struct rotary_encoder *blue_rotary = get_blue_rotary_encoder(control_panel);
  const struct toggles *toggles = get_toggles(control_panel);
  const struct button *red_button = get_red_button(control_panel);
  const struct button *green_button = get_green_button(control_panel);
  const struct selector *blue_selector = get_blue_selector(control_panel);


  if (green_rotary->encoder_delta != 0) {
    change_bpm(this->model, green_rotary->encoder_delta);
  }

  // blue selector to change which sample is played
  if (green_selector->state_count == 0) {
    set_shuffle(this->model, green_selector->selector_state);
  }


  if (blue_rotary->encoder_delta != 0) {
    change_instrument(this->model, blue_rotary->encoder_delta);
  }


  if (toggles->state_count == 0) {
    // something changed- set or unset current instrument on the
    // toggled step.  The toggle_switch defines the position.
    // if the red button is pushed, shift up by 8.
    // Map the left toggle switch to the first step (0) and the
    // right toggle switch to last (7)
    for (int step_num = 0; step_num < 8; ++step_num) {
      if ((toggles->toggles_toggled & (1 << steps_to_toggle_map[step_num]))) {
	if (red_button->button_state) {
	  // red button is pushed
	  toggle_current_triggered_instrument_at_step(this->model, step_num + 8);
	}
	else {
	  printf("toggled toggle num %d (switches: 0x%X) for step %d\n", steps_to_toggle_map[step_num], toggles->toggles_toggled, step_num);
	  toggle_current_triggered_instrument_at_step(this->model, step_num);
	}
      }
    }
  }

  // blue selector to change which sample is played
  if (blue_selector->state_count == 0) {
    change_instrument_sample(this->model, blue_selector->selector_state);
  }

  // green button for start/stop
  if (green_button->button_state == 1 && green_button->state_count == 0) {
    toggle_run_state(this->model);
  }

}

