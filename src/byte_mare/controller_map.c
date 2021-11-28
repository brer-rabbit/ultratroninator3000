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
#include "controller_map.h"
#include "ut3k_pulseaudio.h"


struct controller_map {
  struct model *model;
  struct view_map *view;
  struct ut3k_view *ut3k_view;
};






struct controller_map* create_controller_map(struct model *model, struct view_map *view, struct ut3k_view *ut3k_view) {
  struct controller_map *this;

  this = (struct controller_map*) malloc(sizeof(struct controller_map));
  this->model = model;
  this->view = view;
  this->ut3k_view = ut3k_view;
  clear_view_map(this->view);


  return this;
}

void free_controller_map(struct controller_map *this) {
  free(this);
}


void controller_map_update(struct controller_map *this, uint32_t clock) {
  if (clock & 0b1) {
    // keyscan only valid every 20 ms.  Assume clock is 10ms, so skip
    // every other one.

    update_controls(this->ut3k_view, clock);

    clocktick_model(this->model);
  }

  render_map_display(this->view, clock);
}



/** controller_map callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_map_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct controller_map *this = (struct controller_map*) userdata;
  const struct joystick *joystick = get_joystick(control_panel);


  if (joystick->state_count == 0 && joystick->direction != JOY_CENTERED) {
    move_player(this->model, joystick->direction);
  }


}
