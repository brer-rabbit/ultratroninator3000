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
#include "controller_attract.h"
#include "ut3k_pulseaudio.h"


struct controller_attract {
  struct model *model;
  struct view_attract *view;
  struct ut3k_view *ut3k_view;
  struct clock_text_scroller clock_scroller;
  char *title_attract_ptr;
};



static const int default_scroll_time = 25;
static char *title_attract1 = "BYTE";
static char *title_attract2 = "MARE";



/* start off with accurate front panel state */
static void initialize_model_from_control_panel(struct controller_attract *this, const struct control_panel *control_panel);



struct controller_attract* create_controller_attract(struct model *model, struct view_attract *view, struct ut3k_view *ut3k_view) {
  struct controller_attract *this;

  this = (struct controller_attract*) malloc(sizeof(struct controller_attract));
  this->model = model;
  this->view = view;
  this->ut3k_view = ut3k_view;
  clear_view_attract(this->view);
  this->title_attract_ptr = title_attract1;
  init_clock_text_scroller(&this->clock_scroller,
                           this->title_attract_ptr,
                           default_scroll_time);
  draw_attract(this->view, &this->clock_scroller, f_clock_text_scroller);

  initialize_model_from_control_panel(this, get_control_panel(ut3k_view));

  return this;
}

void free_controller_attract(struct controller_attract *this) {
  free(this);
}


void controller_attract_update(struct controller_attract *this, uint32_t clock) {
  if (clock & 0b1) {
    // keyscan only valid every 20 ms.  Assume clock is 10ms, so skip
    // every other one.
    game_state_t prev_game_state = get_game_state(this->model);

    update_controls(this->ut3k_view, clock);

    clocktick_model(this->model);

    // TODO this will need fixing for multiple controllers
    if (prev_game_state != GAME_ATTRACT) {
      clear_view_attract(this->view);
      this->title_attract_ptr = title_attract1;
      init_clock_text_scroller(&this->clock_scroller,
                               this->title_attract_ptr,
                               default_scroll_time);
      draw_attract(this->view, &this->clock_scroller, f_clock_text_scroller);
    }

    if (text_scroller_is_complete(&this->clock_scroller.scroller_base)) {
      this->title_attract_ptr = this->title_attract_ptr == title_attract1 ?
        title_attract2 : title_attract1;
      toggle_attract_display(this->view);
      init_clock_text_scroller(&this->clock_scroller,
                               this->title_attract_ptr,
                               default_scroll_time);
    }

    draw_attract(this->view, &this->clock_scroller, f_clock_text_scroller);
  }

  render_attract_display(this->view, clock);
}


static void initialize_model_from_control_panel(struct controller_attract *this, const struct control_panel *control_panel) {
}


/** controller_attract callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_attract_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct controller_attract *this = (struct controller_attract*) userdata;
  const struct button *blue_button = get_blue_button(control_panel);
  const struct button *green_button = get_green_button(control_panel);
  const struct button *red_button = get_red_button(control_panel);


  // any button depressed starts the game
  if ((green_button->button_state == 0 && green_button->state_count == 0) ||
      (blue_button->button_state == 0 && blue_button->state_count == 0) ||
      (red_button->button_state == 0 && red_button->state_count == 0)) {
    set_game_start(this->model);
  }

}
