#include <stdlib.h>
#include <stdio.h>

#include "game_controller.h"

struct game_controller {
  struct game_model *model;
  struct game_view *view;
  char *game_exec_to_launch;
};


struct game_controller* create_game_controller(struct game_model *model, struct game_view *view) {
  struct game_controller *this;

  this = (struct game_controller*) malloc(sizeof(struct game_controller));
  this->model = model;
  this->view = view;
  this->game_exec_to_launch = NULL;
  return this;
}

void free_game_controller(struct game_controller *this) {
  free(this);
}

char* get_game_to_launch(struct game_controller *this) {
  return this->game_exec_to_launch;
}

void controller_update(struct game_controller *this) {
  update_view(this->view, this->model);
}

void controller_callback_green_rotary_encoder(int8_t delta, uint8_t button_pushed, uint8_t button_changed, void *userdata) {
  struct game_controller *this = (struct game_controller*) userdata;

  if (button_pushed) { // launch game!
    this->game_exec_to_launch = get_current_executable(this->model);
    return; // don't process any delta from the rotary encoder
  }

  if (delta > 0) {
    next_game(this->model);
  }
  else if (delta < 0) {
    previous_game(this->model);
  }


}