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
#include "controller_flight.h"
#include "ut3k_pulseaudio.h"

typedef enum { FLIGHT_UNSET, FLIGHT_SET, FLIGHT_IN_PROGRESS, FLIGHT_COMPLETE } flight_state_t;

struct controller_flight {
  struct model *model;
  struct view_flight *view;
  struct ut3k_view *ut3k_view;

  flight_state_t flight_state;
  struct clock_text_scroller clock_scroller_green;
};


static const int default_scroll_time = 18;
static const char *intro = "    AVOID WALLS - DON'T CRASH    ";
static const char *complete = "    ARRIVING AT DESTINATION    ";



struct controller_flight* create_controller_flight(struct model *model, struct view_flight *view, struct ut3k_view *ut3k_view) {
  struct controller_flight *this;

  this = (struct controller_flight*) malloc(sizeof(struct controller_flight));
  this->model = model;
  this->view = view;
  this->ut3k_view = ut3k_view;
  clear_view_flight(this->view);

  this->flight_state = FLIGHT_UNSET;
  return this;
}

void free_controller_flight(struct controller_flight *this) {
  free(this);
}


void controller_flight_init(struct controller_flight *this) {
  clear_view_flight(this->view);
  init_clock_text_scroller(&this->clock_scroller_green,
                           intro,
                           default_scroll_time);
  set_flight_messaging(this->view, &this->clock_scroller_green, f_clock_text_scroller);
  this->flight_state = FLIGHT_SET;
}


void controller_flight_update(struct controller_flight *this, uint32_t clock) {
  if (clock & 0b1) {
    // keyscan only valid every 20 ms.  Assume clock is 10ms, so skip
    // every other one.
    update_controls(this->ut3k_view, clock);
  }


  if (this->flight_state == FLIGHT_COMPLETE) {
    if (text_scroller_is_complete(&this->clock_scroller_green.scroller_base)) {
      this->flight_state = FLIGHT_UNSET;
      clear_view_flight(this->view);
      // TODO: set model state
    }
  }
  else if (this->flight_state == FLIGHT_SET) {
    if (text_scroller_is_complete(&this->clock_scroller_green.scroller_base)) {
      this->flight_state = FLIGHT_IN_PROGRESS;
      clear_view_flight(this->view);
    }
  }
  else if (this->flight_state == FLIGHT_IN_PROGRESS) {
    if (clock & 0b1) {
      clocktick_model(this->model);
    }

    const struct player *player = get_player(this->model);
    const struct flight_path *flight_path = get_flight_path(this->model);
    draw_flight_tunnel(this->view, flight_path, player, clock);

    if (is_flight_path_complete(this->model)) {
      this->flight_state = FLIGHT_COMPLETE;
      init_clock_text_scroller(&this->clock_scroller_green,
                               complete,
                               default_scroll_time);
      set_flight_messaging(this->view, &this->clock_scroller_green, f_clock_text_scroller);

    }
  }

  render_flight_display(this->view, clock);
}



/** controller_flight callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_flight_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct controller_flight *this = (struct controller_flight*) userdata;
  const struct joystick *joystick = get_joystick(control_panel);
  const struct rotary_encoder *red_rotary_encoder = get_red_rotary_encoder(control_panel);


  if (this->flight_state != FLIGHT_IN_PROGRESS) {
    return;
  }

  if (joystick->state_count == 0 && joystick->direction == JOY_LEFT) {
    flight_move_player(this->model, -1);
  }
  else if (joystick->state_count == 0 && joystick->direction == JOY_RIGHT) {
    flight_move_player(this->model, 1);
  }

  if (red_rotary_encoder->encoder_delta != 0) {
    flight_move_player(this->model, red_rotary_encoder->encoder_delta);
  }

}
