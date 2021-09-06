/** initialize_game_model
 *
 * get the backpacks up and going. Clear their mem first.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "game_model.h"

struct game_model {
  char *game_executable;
  char *display_name[3];
};



/* create the model.
 *
 */
struct game_model* create_game_model() {
  return (struct game_model*)malloc(sizeof(struct game_model));
}

void free_game_model(struct game_model *model) {
  return free(model);
}



int set_green_string(struct game_model *this, char *display_string) {
  this->display_name[0] = display_string;
  return 0;
}

char* get_green_string(struct game_model *this) {
  if (this) {
    return this->display_name[0];
  }
  return NULL;
}

int set_blue_string(struct game_model *this, char *display_string) {
  this->display_name[1] = display_string;
  return 0;
}

char* get_blue_string(struct game_model *this) {
  if (this) {
    return this->display_name[1];
  }
  return NULL;
}


int set_red_string(struct game_model *this, char *display_string) {
  this->display_name[2] = display_string;
  return 0;
}

char* get_red_string(struct game_model *this) {
  if (this) {
    return this->display_name[2];
  }
  return NULL;
}
