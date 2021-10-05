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
#include "hex_inv_ader.h"


static const int32_t in_state_next_event_time = 22;
static const int32_t transition_to_next_state_time = 800;
static const int32_t game_over_to_attract_time = 1000;
static const int32_t attract_sample_play_time = 2000;

struct controller {
  struct model *model;
  struct ut3k_view *view;

  int32_t clock_to_next_event;
  int32_t clock_to_next_state;
};


/* start off with accurate front panel state */
static void controller_callback_control_panel_gameplay(const struct control_panel *control_panel, void *userdata);
static void controller_callback_control_panel_attractmode(const struct control_panel *control_panel, void *userdata);
static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel);
static void controller_update_game_over(struct controller *this, uint32_t clock);
static void controller_update_game_playing(struct controller *this, uint32_t clock);
static void controller_update_level_up(struct controller *this, uint32_t clock);
static void controller_update_attract(struct controller *this, uint32_t clock);


/* functions ---------------------------------------------------*/

struct controller* create_controller(struct model *model, struct ut3k_view *view) {
  struct controller *this;

  this = (struct controller*) malloc(sizeof(struct controller));
  this->model = model;
  this->view = view;
  this->clock_to_next_event = 0;
  this->clock_to_next_state = 0;

  register_control_panel_listener(this->view, controller_callback_control_panel_attractmode, this);
  game_attract(this->model);
  initialize_model_from_control_panel(this, get_control_panel(view));

  return this;
}

void free_controller(struct controller *this) {
  free(this);
}


void controller_update(struct controller *this, uint32_t clock) {
  game_state_t game_state = get_game_state(this->model), new_game_state;
  
  switch (game_state) {
  case GAME_OVER:
    controller_update_game_over(this, clock);
    break;
  case GAME_PLAYING:
    controller_update_game_playing(this, clock);
    break;
  case GAME_LEVEL_UP:
    controller_update_level_up(this, clock);
    break;
  case GAME_ATTRACT:
    controller_update_attract(this, clock);
  }

  update_view(this->view, get_display_strategy(this->model), clock);

  new_game_state = get_game_state(this->model);
  if (new_game_state != game_state) { // state changed
    this->clock_to_next_event = in_state_next_event_time;

    if (new_game_state == GAME_OVER) {
      this->clock_to_next_state = game_over_to_attract_time;
    }
    else {
      this->clock_to_next_state = transition_to_next_state_time;
    }
  }
}


static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel) {
  const struct toggles *toggles = get_toggles(control_panel);
  // bug (feature): this plays the toggled sound on startup.
  // could easily fix, but not really wanting to...
  set_player_laser_value(this->model, toggles->toggles_state & 0xF);
}


/** controller callbacks
 * 
 * implements f_view_control_panel_updates
 */
static void controller_callback_control_panel_gameplay(const struct control_panel *control_panel, void *userdata) {
  struct controller *this = (struct controller*) userdata;


  const struct rotary_encoder *red_rotary_encoder = get_red_rotary_encoder(control_panel);
  if (red_rotary_encoder->encoder_delta == -1) {
    player_left(this->model);
  }
  else if (red_rotary_encoder->encoder_delta == 1) {
    player_right(this->model);
  }

  const struct toggles *toggles = get_toggles(control_panel);
  if (toggles->state_count == 0) {
    set_player_laser_value(this->model, toggles->toggles_state & 0xF);
  }


  const struct button *red_button = get_red_button(control_panel);
  const struct button *blue_button = get_blue_button(control_panel);
  if (blue_button->button_state == 1) {
    // blue button: show the actual Hex digit the player's laser is set to
    // if the model allows it-
    set_player_as_hexdigit(this->model);
  }
  else {
    set_player_as_glyph(this->model);
  }

  if (red_button->state_count == 0 && red_button->button_state == 1) {
    // red button: fire the laser
    set_player_laser_fired(this->model);
  }

}


static void controller_callback_control_panel_attractmode(const struct control_panel *control_panel, void *userdata) {
  struct controller *this = (struct controller*) userdata;

  // either button starts game
  const struct button *red_button = get_red_button(control_panel);
  const struct button *blue_button = get_blue_button(control_panel);
  if ((blue_button->state_count == 0 && blue_button->button_state == 1) ||
      (red_button->state_count == 0 && red_button->button_state == 1)) {
    // reset listener for game start
    register_control_panel_listener(this->view, controller_callback_control_panel_gameplay, this);
    game_start(this->model);
    const struct toggles *toggles = get_toggles(control_panel);
    set_player_laser_value(this->model, toggles->toggles_state & 0xF);
  }

}



static void controller_update_game_playing(struct controller *this, uint32_t clock) {
  int invader_id_collision, invader_id_destroyed;
  int shield_remaining;

  clocktick_invaders(this->model);
  clocktick_player_laser(this->model);

  invader_id_collision = check_collision_invaders_player(this->model);
  // most of this ought to be refactored into model
  if (invader_id_collision >= 0) {
    // collion, shield takes a hit
    shield_remaining = player_shield_hit(this->model);
    destroy_invader(this->model, invader_id_collision);
    if (shield_remaining == 0) {
      // game over!
      ut3k_play_sample(gameover_soundkey);
      game_over(this->model);
    }
    else {
      // destroy invader, no points
      if (shield_remaining == 1) {
	ut3k_play_sample(shieldlow_soundkey);
      }
      else {
	ut3k_play_sample(shieldhit_soundkey);
      }
    }
  }

  // the check collision for the laser either returns a handle to the
  // alien destroyed, -1 for a miss, or -2 for a shielded hit
  invader_id_destroyed = check_collision_player_laser_to_aliens(this->model);
  if (invader_id_destroyed >= 0) {
    destroy_invader(this->model, invader_id_destroyed);
    ut3k_play_sample(laser_hit_invader_soundkey);
  }
  else if (invader_id_destroyed == -3) {
    ut3k_play_sample(laser_hit_invader_shield_destroyed_soundkey);
  }
  else if (invader_id_destroyed == -2) {
    ut3k_play_sample(laser_hit_shielded_invader_soundkey);
  }

  // the model will transition state if and only if conditions are met
  game_level_up(this->model);
}


static void controller_update_game_over(struct controller *this, uint32_t clock) {
  if (--this->clock_to_next_event <= 0) {
    this->clock_to_next_event = in_state_next_event_time;
    game_over_scroll(this->model);
  }

  if (--this->clock_to_next_state <= 0) {
    register_control_panel_listener(this->view, controller_callback_control_panel_attractmode, this);
    game_attract(this->model);
  }
}



static void controller_update_level_up(struct controller *this, uint32_t clock) {
  if (--this->clock_to_next_event <= 0) {
    this->clock_to_next_event = in_state_next_event_time;
    level_up_scroll(this->model);
  }

  --this->clock_to_next_state;
  if (this->clock_to_next_state <= 0) {
    game_resume(this->model);
  }
  else if (this->clock_to_next_state == 50) { // getting close, prep player
      ut3k_play_sample(levelup_soundkey);
  }
  
}

static void controller_update_attract(struct controller *this, uint32_t clock) {

  if (--this->clock_to_next_event <= 0) {
    game_attract_scroll(this->model);
    this->clock_to_next_event = in_state_next_event_time;
  }

  // no timed exit from this state-- stay in attract mode
  if (--this->clock_to_next_state <= 0) {
    this->clock_to_next_state = attract_sample_play_time;
    ut3k_play_sample(attract_soundkey);
  }  
}
