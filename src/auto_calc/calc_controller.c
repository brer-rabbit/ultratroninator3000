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
#include "calc_controller.h"

struct calc_controller {
  struct calc_model *model;
  struct ut3k_view *view;

  int32_t green_cycle_time;
  int32_t blue_cycle_time;

  uint8_t manual_green_register_flag;
  int32_t green_remaining_time;
  int32_t blue_remaining_time;
};


/* start off with accurate front panel state */
static void initialize_model_from_control_panel(struct calc_controller *this, const struct control_panel *control_panel);


// start a default of 120 cycles (~3 seconds) between integer changes
static const int32_t default_cycle_time = 50;


struct calc_controller* create_calc_controller(struct calc_model *model, struct ut3k_view *view) {
  struct calc_controller *this;

  this = (struct calc_controller*) malloc(sizeof(struct calc_controller));
  this->model = model;
  this->view = view;

  this->green_cycle_time = default_cycle_time * 0.95;
  this->blue_cycle_time = default_cycle_time * 1.05;
  this->green_remaining_time = this->green_cycle_time;
  this->blue_remaining_time = this->blue_cycle_time;
  this->manual_green_register_flag = 0;

  initialize_model_from_control_panel(this, get_control_panel(view));

  return this;
}

void free_calc_controller(struct calc_controller *this) {
  free(this);
}


void controller_update(struct calc_controller *this, uint32_t clock) {
  if (--this->green_remaining_time <= 0 && this->manual_green_register_flag == 0) {
    // update green
    update_green_register(this->model);
    this->green_remaining_time = this->green_cycle_time;
  }
  if (--this->blue_remaining_time <= 0) {
    // update blue
    update_blue_register(this->model);
    this->blue_remaining_time = this->blue_cycle_time;
  }

  update_red_register(this->model);

  update_view(this->view, get_display_strategy(this->model), clock);
}


static void initialize_model_from_control_panel(struct calc_controller *this, const struct control_panel *control_panel) {
  const struct selector *green_selector = get_green_selector(control_panel);
  const struct selector *blue_selector = get_blue_selector(control_panel);
  const struct toggles *toggles = get_toggles(control_panel);

  
  switch(green_selector->selector_state) {
  case SELECTOR_THREE:
    set_calc_function(this->model, f_calc_and);
    break;
  case SELECTOR_TWO:
    set_calc_function(this->model, f_calc_or);
    break;
  case SELECTOR_ONE:
    set_calc_function(this->model, f_calc_sub);
    break;
  case SELECTOR_ZERO:
  default:
    set_calc_function(this->model, f_calc_add);
  }

  switch(blue_selector->selector_state) {
  case SELECTOR_THREE:
    set_next_value_blue_function(this->model, f_next_random);
    break;
  case SELECTOR_TWO:
    set_next_value_blue_function(this->model, f_next_times_two);
    break;
  case SELECTOR_ONE:
    set_next_value_blue_function(this->model, f_next_minus_one);
    break;
  case SELECTOR_ZERO:
  default:
    set_next_value_blue_function(this->model, f_next_plus_one);
  }

}


/** controller callback
 * 
 * implements f_view_control_panel_updates
 */
void controller_callback_control_panel(const struct control_panel *control_panel, void *userdata) {
  struct calc_controller *this = (struct calc_controller*) userdata;

  const struct rotary_encoder *green_rotary_encoder = get_green_rotary_encoder(control_panel);
  const struct rotary_encoder *blue_rotary_encoder = get_blue_rotary_encoder(control_panel);
  const struct selector *green_selector = get_green_selector(control_panel);
  const struct selector *blue_selector = get_blue_selector(control_panel);
  const struct toggles *toggles = get_toggles(control_panel);
  const struct button *red_button = get_red_button(control_panel);



  // green rotary encoder is speed of green register changes --
  // unless it's in manual mode
  if (green_rotary_encoder->encoder_delta > 0) {
    if (this->green_cycle_time < 2000) {
      this->green_cycle_time += 5;
      printf("green_cycle_time inc %d\n", this->green_cycle_time);
    }
  }
  else if (green_rotary_encoder->encoder_delta < 0) {
    if (this->green_cycle_time > 1) {
      this->green_cycle_time -= 5;
      printf("green_cycle_time dec %d\n", this->green_cycle_time);
    }
    // since we're increasing speed: auto-move to next
    this->green_remaining_time = 0;
  }

  // pushing green rotary push button toggles auto/manual mode
  if (green_rotary_encoder->button.state_count == 0 &&
      green_rotary_encoder->button.button_state) {
    this->manual_green_register_flag = !this->manual_green_register_flag;

    // and set them if we're just entering the state
    if (this->manual_green_register_flag) {
      set_green_register(this->model, toggles->toggles_state);
    }
  }



  // blue rotary encoder is speed of blue register changes
  if (blue_rotary_encoder->encoder_delta > 0) {
    if (this->blue_cycle_time < 2000) {
      this->blue_cycle_time += 5;
      printf("blue_cycle_time inc %d\n", this->blue_cycle_time);
    }

  }
  else if (blue_rotary_encoder->encoder_delta < 0) {
    if (this->blue_cycle_time > 1) {
      this->blue_cycle_time -= 5;
      printf("blue_cycle_time dec %d\n", this->blue_cycle_time);
    }
    // since we're increasing speed: auto-move to next
    this->blue_remaining_time = 0;
  }



  // green select changes the calculation function
  if (green_selector->state_count == 0) {
    switch(green_selector->selector_state) {
    case SELECTOR_THREE:
      set_calc_function(this->model, f_calc_and);
      break;
    case SELECTOR_TWO:
      set_calc_function(this->model, f_calc_or);
      break;
    case SELECTOR_ONE:
      set_calc_function(this->model, f_calc_sub);
      break;
    case SELECTOR_ZERO:
    default:
      set_calc_function(this->model, f_calc_add);
    }
  }


  // blue select changes the next blue value function
  if (blue_selector->state_count == 0) {
    switch(blue_selector->selector_state) {
    case SELECTOR_THREE:
      set_next_value_blue_function(this->model, f_next_random);
      break;
    case SELECTOR_TWO:
      set_next_value_blue_function(this->model, f_next_times_two);
      break;
    case SELECTOR_ONE:
      set_next_value_blue_function(this->model, f_next_minus_one);
      break;
    case SELECTOR_ZERO:
    default:
      set_next_value_blue_function(this->model, f_next_plus_one);
    }
  }



  // Red button swaps the blue & green registers
  if (red_button->state_count == 0 && red_button->button_state) {
    swap_registers(this->model);
  }



  if (toggles->state_count == 0 && this->manual_green_register_flag) {
    set_green_register(this->model, toggles->toggles_state);
  }


}
