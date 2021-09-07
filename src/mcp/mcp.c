#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "game_model.h"
#include "game_view.h"
#include "game_controller.h"


/** Master Control Program
 *
 * Get env var GAMES_LOCAL
 * Read dir specified from GAMES_LOCAL for all executable files
 * map filenames to something that can be displayed
 * setup handler for kill switch
 * listen for user input to scroll through list
 * fork & exec selected program
 * waitpid on child process
 * signal handler for kill switch sends signal to child process
 * loop back to listening for user input
 */


int main(int argc, char **argv) {

  struct game_model *model;
  struct game_view *view;
  struct game_controller *controller;


  model = create_game_model();
  if (model == NULL) {
    printf("error creating game model\n");
    return 1;
  }

  view = create_alphanum_game_view();
  if (view == NULL) {
    printf("error creating game view\n");
    return 1;
  }

  controller = create_game_controller(model, view);
  if (controller == NULL) {
    printf("error creating game controller\n");
    return 1;
  }


  register_green_encoder_listener(view, controller_callback_green_rotary_encoder, controller);

  // scroll through all the games
  for (int i = 0; i < 500; ++i) {
    // should this function even exist or just rely on callback?
    // could instead just make this a loop around update_view?
    controller_update(controller);
    usleep(35000);
  }

  


  free_game_controller(controller);
  free_game_view(view);
  free_game_model(model);

  return 0;
}



