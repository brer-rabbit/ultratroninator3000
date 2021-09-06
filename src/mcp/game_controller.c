#include <stdlib.h>

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

int free_game_controller(struct game_controller *this) {
  free(this);
  return 0;
}

void controller_update_data(struct game_controller *this, char *redstr) {
  set_red_string(this->model, redstr);
  display_view(this->view, this->model);
}
