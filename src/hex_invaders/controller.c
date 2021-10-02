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

struct controller {
  struct model *model;
  struct ut3k_view *view;

  uint8_t clock_to_next_state;
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
  int invader_id_collision, invader_id_destroyed;
  int shield_remaining;

  clocktick_invaders(this->model);

  invader_id_collision = check_collision_invaders_player(this->model);
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

  invader_id_destroyed = check_collision_player_laser_to_aliens(this->model);
  if (invader_id_destroyed >= 0) {
    destroy_invader(this->model, invader_id_destroyed);
    ut3k_play_sample(laser_hit_invader_soundkey);
  }
  else if (invader_id_destroyed == -2) {
    ut3k_play_sample(laser_hit_shielded_invader_soundkey);
  }

    
  // invaders aren't firing lasers
  //check_collision_invaders_laser_to_player(this->model);

  clocktick_player_laser(this->model);

  update_view(this->view, get_display_strategy(this->model), clock);
}


static void initialize_model_from_control_panel(struct controller *this, const struct control_panel *control_panel) {
  start_invader(this->model);
  const struct toggles *toggles = get_toggles(control_panel);
  set_player_laser_value(this->model, toggles->toggles_state & 0xF);
}


/** controller callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
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
    ut3k_play_sample(laser_toggled_soundkey);
  }


  const struct button *red_button = get_red_button(control_panel);
  if (red_button->button_state == 1) {
    // red button: show the actual Hex digit the player's laser is set to
    // if the model allows it-
    if (set_player_as_hexdigit(this->model)) {
      ut3k_play_sample(showhex_soundkey);
    }
  }
  else {
    if (set_player_as_glyph(this->model)) {
      // play revert sound
      ut3k_play_sample(hidehex_soundkey);
    }
  }

  const struct button *blue_button = get_blue_button(control_panel);
  if (blue_button->state_count == 0 && blue_button->button_state == 1) {
    // red button: show the actual Hex digit the player's laser is set to
    // if the model allows it-
    if (set_player_laser_fired(this->model)) {
      // play fire laser sound
      ut3k_play_sample(playerfire_soundkey);
    }
  }

}
