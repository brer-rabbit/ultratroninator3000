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
  struct view *view;
  struct ut3k_view *ut3k_view;
  struct manual_text_scroller manual_scroller;
  struct clock_text_scroller clock_scroller;
};






/* start off with accurate front panel state */
static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel);



struct controller* create_controller(struct model *model, struct view *view, struct ut3k_view *ut3k_view) {
  struct controller *this;

  this = (struct controller*) malloc(sizeof(struct controller));
  this->model = model;
  this->view = view;
  this->ut3k_view = ut3k_view;

  initialize_model_from_control_panel(this, get_control_panel(ut3k_view));

  return this;
}

void free_controller(struct controller *this) {
  free(this);
}


void controller_update(struct controller *this, uint32_t clock) {
  const struct paddle *player1_paddle;
  const struct paddle *player2_paddle;
  const struct ball *ball;

  // given we can only update controls every other clock, it's
  // not fair to update the model.  That's the only processing done
  // here, so update is only required every other clocks
  if (clock & 0b1) {
    return;
  }

  update_controls(this->ut3k_view, clock);
  clocktick_model(this->model);

  player1_paddle = get_player1(this->model);
  player2_paddle = get_player2(this->model);
  draw_player1_paddle(this->view, player1_paddle->y_position);
  draw_player2_paddle(this->view, player2_paddle->y_position);

  ball = get_ball(this->model);
  draw_ball(this->view, ball->x_position, ball->y_position);
  draw_player1_score(this->view, player1_paddle->score);
  draw_player2_score(this->view, player2_paddle->score);

  render_view(this->view, clock);
}


static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel) {
}


/** controller callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct controller *this = (struct controller*) userdata;
  const struct button *blue_button = get_blue_button(control_panel);
  const struct button *red_button = get_red_button(control_panel);

  const struct rotary_encoder *green_rotary_encoder = get_green_rotary_encoder(control_panel);
  const struct rotary_encoder *blue_rotary_encoder = get_blue_rotary_encoder(control_panel);
  const struct rotary_encoder *red_rotary_encoder = get_red_rotary_encoder(control_panel);
  
  player1_move(this->model, red_rotary_encoder->encoder_delta);
  player2_move(this->model, blue_rotary_encoder->encoder_delta);


  if (red_button->button_state == 1 && red_button->state_count == 0) {
    player1_serve(this->model);
  }

  if (blue_button->button_state == 1 && blue_button->state_count == 0) {
    player2_serve(this->model);
  }

}
