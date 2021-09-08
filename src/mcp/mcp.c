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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libconfig.h>

#include "mcp.h"
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

// configurable duration for event loop; default to 35ms
#define EVENT_LOOP_DURATION_TIME_KEY "event_loop_duration"
#define EVENT_LOOP_DURATION_TIME_DEFAULT 35


static void launch_game(char *executable, char **envp) {
  char *argv[2];
  argv[0] = executable;
  argv[1] = NULL;

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


static char* run_mvc(config_t *cfg) {
  struct game_model *model;
  struct game_view *view;
  struct game_controller *controller;
  char *executable = NULL;
  struct timeval tval_controller_start, tval_controller_end, tval_controller_time, tval_fixed_loop_time, tval_sleep_time;
  int loop_time_ms;

  if (cfg != NULL &&
      config_lookup_int(cfg, EVENT_LOOP_DURATION_TIME_KEY, &loop_time_ms)) {
    printf("using config loop duration of %d\n", loop_time_ms);
    tval_fixed_loop_time.tv_sec = 0;
    tval_fixed_loop_time.tv_usec = loop_time_ms * 1000;
  }
  else {
    printf("No loop duration setting in configuration file.\n");
    tval_fixed_loop_time.tv_sec = 0;
    tval_fixed_loop_time.tv_usec = EVENT_LOOP_DURATION_TIME_DEFAULT * 1000;
  }

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


  // init
  tval_sleep_time.tv_sec = 0;
  tval_sleep_time.tv_usec = 0;
  

  // scroll through all the games.  Try and nail the loop timewise.
  while (executable == NULL) {
    // do the usleep first, otherwise the game launch is delayed by ~30 ms

    if (tval_sleep_time.tv_sec != 0) {
       // this really shouldn't happen... loop takes ~6 ms.  Expect 0.025-0.029
      printf("controller took wayyyy too long\n");
    }
    else {
      // sleep for the fixed loop time minux the time for the controller
      usleep(tval_sleep_time.tv_usec);
    }

    // should this function even exist or just rely on callback?
    // could instead just make this a loop around update_view?
    gettimeofday(&tval_controller_start, NULL);

    controller_update(controller);
    executable = get_game_to_launch(controller);
    gettimeofday(&tval_controller_end, NULL);
    // tval_controller_time is duration of controller_update
    timersub(&tval_controller_end, &tval_controller_start, &tval_controller_time);
    timersub(&tval_fixed_loop_time, &tval_controller_time, &tval_sleep_time);

  }


  sleep(2);

  free_game_controller(controller);
  free_game_view(view);
  free_game_model(model);

  return executable;
}



int main(int argc, char **argv, char **envp) {
  char *executable;
  char *games_directory, config_filename[128];
  config_t *cfg;

  cfg = (config_t*)malloc(sizeof(config_t));

  config_init(cfg);

  if (! (games_directory = getenv(GAMES_LOCAL_ENV_VAR)) ) {
    games_directory = DEFAULT_GAMES_BASEDIR;
  }

  snprintf(config_filename, 128, "%s%s", games_directory, CONFIG_FILENAME);

  printf("looking for config: %s\n", config_filename);

  /* configuration file ought to exist here... */
  if(! config_read_file(cfg, config_filename))
  {
    fprintf(stderr, "no config file?\n%s:%d - %s\n", config_error_file(cfg),
            config_error_line(cfg), config_error_text(cfg));
    config_destroy(cfg);
    free(cfg);
    cfg = NULL;
  }


  for (int i = 0; i < 3; ++i) {
    executable = run_mvc(cfg);
    launch_game(executable, envp);
  }

  if (cfg) {
    config_destroy(cfg);
    free(cfg);
  }


  return 0;
}


