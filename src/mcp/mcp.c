#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
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


static void launch_game(char *executable, char **envp) {
  char *argv[2];
  argv[0] = executable;
  argv[1] = NULL;

  printf("launching %s\n", executable);
  pid_t parent = getpid();
  pid_t pid = fork();

  if (pid == -1) {
    printf("error, failed to fork()!\n");
  } 
  else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
    printf("waited on %d with return status %d\n", pid, status);
  }
  else  {
    // we are the child
    execve(executable, argv, envp);
    _exit(EXIT_FAILURE);   // exec never returns
  }
}


static char* run_mvc() {
  struct game_model *model;
  struct game_view *view;
  struct game_controller *controller;
  char *executable = NULL;

  model = create_game_model();
  if (model == NULL) {
    printf("error creating game model\n");
    return NULL;
  }

  view = create_alphanum_game_view();
  if (view == NULL) {
    printf("error creating game view\n");
    return NULL;
  }

  controller = create_game_controller(model, view);
  if (controller == NULL) {
    printf("error creating game controller\n");
    return NULL;
  }


  register_green_encoder_listener(view, controller_callback_green_rotary_encoder, controller);

  // scroll through all the games
  while (executable == NULL) {
    // should this function even exist or just rely on callback?
    // could instead just make this a loop around update_view?
    controller_update(controller);
    executable = get_game_to_launch(controller);

    usleep(35000);
  }


  free_game_controller(controller);
  free_game_view(view);
  free_game_model(model);

  return executable;
}



int main(int argc, char **argv, char **envp) {
  char *executable;


  for (int i = 0; i < 3; ++i) {
    executable = run_mvc();
    launch_game(executable, envp);
  }

  return 0;
}


