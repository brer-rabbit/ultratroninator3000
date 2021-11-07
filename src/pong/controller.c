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
  struct clock_text_scroller clock_scroller_green;
  char messaging_green[32];
};


static const int default_scroll_time = 13;
static const char *title_attract = "PONG";


/* start off with accurate front panel state */
static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel);



struct controller* create_controller(struct model *model, struct view *view, struct ut3k_view *ut3k_view) {
  struct controller *this;

  this = (struct controller*) malloc(sizeof(struct controller));
  this->model = model;
  this->view = view;
  this->ut3k_view = ut3k_view;

  initialize_model_from_control_panel(this, get_control_panel(ut3k_view));
  
  init_clock_text_scroller(&this->clock_scroller_green,
                           title_attract,
                           default_scroll_time);

  draw_attract(this->view, &this->clock_scroller_green, f_clock_text_scroller);

  return this;
}

void free_controller(struct controller *this) {
  free(this);
}


void controller_update(struct controller *this, uint32_t clock) {
  const struct player *player1;
  const struct player *player2;
  const struct ball *ball;
  // game_state- it may change after clockticking the model.  Identify state transition.

  // given we can only update controls every other clock, it's
  // not fair to update the model.  That's the only processing done
  // here, so update is only required every other clocks
  if (clock & 0b1) {
    return;
  }

  game_state_t prev_game_state = get_game_state(this->model);

  update_controls(this->ut3k_view, clock);
  switch (clocktick_model(this->model)) {
  case GAME_OVER:
    player1 = get_player1(this->model);
    player2 = get_player2(this->model);

    if (prev_game_state != GAME_OVER) {
      clear_view(this->view);
      snprintf(this->messaging_green, 32, "    PLAYER %d WINS    ",
               player1->score > player2->score ? 1 : 2);
      init_clock_text_scroller(&this->clock_scroller_green,
                               this->messaging_green,
                               default_scroll_time);
    }
               
    draw_gameover(this->view, &this->clock_scroller_green, f_clock_text_scroller, player1->score, player2->score);

    if (text_scroller_is_complete(&this->clock_scroller_green.scroller_base)) {
      text_scroller_reset(&this->clock_scroller_green.scroller_base);
    }

    draw_player1_score(this->view, player1->score);
    draw_player2_score(this->view, player2->score);

    break;
  case GAME_PLAY:
  case GAME_SERVE:
    if (prev_game_state != GAME_PLAY ||
        prev_game_state != GAME_SERVE) {
      clear_view(this->view);
    }
    player1 = get_player1(this->model);
    player2 = get_player2(this->model);
    draw_player1_paddle(this->view, player1->y_position, player1->handicap);
    draw_player2_paddle(this->view, player2->y_position, player2->handicap);

    ball = get_ball(this->model);
    draw_ball(this->view, ball->x_position, ball->y_position);
    draw_player1_score(this->view, player1->score);
    draw_player2_score(this->view, player2->score);
    break;
  case GAME_ATTRACT:
    if (prev_game_state != GAME_ATTRACT) {
      clear_view(this->view);
      init_clock_text_scroller(&this->clock_scroller_green,
                               title_attract,
                               default_scroll_time);
      draw_attract(this->view, &this->clock_scroller_green, f_clock_text_scroller);
    }

    if (text_scroller_is_complete(&this->clock_scroller_green.scroller_base)) {
      text_scroller_reset(&this->clock_scroller_green.scroller_base);
      draw_attract(this->view, &this->clock_scroller_green, f_clock_text_scroller);
    }

    break;
  default:
    break;
  }

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
  const struct selector *blue_selector = get_blue_selector(control_panel);
  const struct selector *green_selector = get_green_selector(control_panel);
  const struct rotary_encoder *blue_rotary_encoder = get_blue_rotary_encoder(control_panel);
  const struct rotary_encoder *red_rotary_encoder = get_red_rotary_encoder(control_panel);
  
  player1_move(this->model, red_rotary_encoder->encoder_delta);
  player2_move(this->model, blue_rotary_encoder->encoder_delta);


  if (red_button->button_state == 1 && red_button->state_count == 0) {
    player1_button_pushed(this->model);
  }

  if (blue_button->button_state == 1 && blue_button->state_count == 0) {
    player2_button_pushed(this->model);
  }

  if (green_selector->state_count == 0) {
    switch (green_selector->selector_state) {
    case SELECTOR_ZERO:
      set_player1_handicap(this->model, HANDICAP_EXPERT);
      break;
    case SELECTOR_ONE:
      set_player1_handicap(this->model, HANDICAP_MED2);
      break;
    case SELECTOR_TWO:
      set_player1_handicap(this->model, HANDICAP_MED1);
      break;
    case SELECTOR_THREE:
      set_player1_handicap(this->model, HANDICAP_NOVICE);
      break;
    default:
      set_player1_handicap(this->model, HANDICAP_EXPERT);
      break;
    }
  }

  if (blue_selector->state_count == 0) {
    switch (blue_selector->selector_state) {
    case SELECTOR_ZERO:
      set_player2_handicap(this->model, HANDICAP_EXPERT);
      break;
    case SELECTOR_ONE:
      set_player2_handicap(this->model, HANDICAP_MED2);
      break;
    case SELECTOR_TWO:
      set_player2_handicap(this->model, HANDICAP_MED1);
      break;
    case SELECTOR_THREE:
      set_player2_handicap(this->model, HANDICAP_NOVICE);
      break;
    }
  }

}
