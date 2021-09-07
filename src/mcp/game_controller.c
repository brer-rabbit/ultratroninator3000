#include <stdlib.h>
#include <stdio.h>

#include "game_controller.h"

struct game_controller {
  struct game_model *model;
  struct game_view *view;
};


struct game_controller* create_game_controller(struct game_model *model, struct game_view *view) {
  struct game_controller *this;

  this = (struct game_controller*) malloc(sizeof(struct game_controller));
  this->model = model;
  this->view = view;
  return this;
}

void free_game_controller(struct game_controller *this) {
  free(this);
}

void controller_update(struct game_controller *this) {
  update_view(this->view, this->model);
}

void controller_callback_green_rotary_encoder(int8_t delta, uint8_t button_pushed, uint8_t button_changed, void *userdata) {
  struct game_controller *controller = (struct game_controller*) userdata;
  if (button_changed) {
    printf("button is %s and %s\n",
	   button_pushed ? "pushed" : "not pushed",
	   button_changed ? "changed" : "didn't change");
  }

  if (delta != 0) {
    printf("delta: %d\n", delta);
  }
}
