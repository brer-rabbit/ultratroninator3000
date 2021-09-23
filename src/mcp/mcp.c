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
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <libconfig.h>

#include "mcp.h"
#include "game_model.h"
#include "game_controller.h"
#include "ut3k_view.h"


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
#define CONFIG_EVENT_LOOP_DURATION_TIME_KEY "event_loop_duration"
#define EVENT_LOOP_DURATION_TIME_DEFAULT 35
#define CONFIG_SOUND_LIST_KEY "sound_list"


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
  struct ut3k_view *view;
  struct game_controller *controller;
  char *executable = NULL;
  struct timeval tval_controller_start, tval_controller_end, tval_controller_time, tval_fixed_loop_time, tval_sleep_time;
  int loop_time_ms;

  if (cfg != NULL &&
      config_lookup_int(cfg, CONFIG_EVENT_LOOP_DURATION_TIME_KEY, &loop_time_ms)) {
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

  view = create_alphanum_ut3k_view();
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
  

  // event loop
  // scroll through all the games.  Try and nail the loop timewise.
  while (executable == NULL) {
    // do the usleep first, otherwise the game launch is delayed by way too long

    if (tval_sleep_time.tv_sec != 0) {
       // this really shouldn't happen... loop takes <8 ms
      printf("controller took %ld.%06ld; this is longer than event loop of %ld.%06ld seconds\n", tval_controller_time.tv_sec, tval_controller_time.tv_usec, tval_fixed_loop_time.tv_sec, tval_fixed_loop_time.tv_usec);
    }
    else {
      // sleep for the fixed loop time minus the time for the controller
      usleep(tval_sleep_time.tv_usec);
    }

    // should this function even exist or just rely on callback?
    // could instead just make this a loop around update_view?
    gettimeofday(&tval_controller_start, NULL);

    controller_update(controller);
    executable = get_game_to_launch(controller);
    ut3k_pa_mainloop_iterate();

    gettimeofday(&tval_controller_end, NULL);
    // tval_controller_time is duration of controller_update
    // tval_fixed_loop_time - tval_controller_time ==> time to sleep between iterations
    timersub(&tval_controller_end, &tval_controller_start, &tval_controller_time);
    timersub(&tval_fixed_loop_time, &tval_controller_time, &tval_sleep_time);

  }


  sleep(2);

  free_game_controller(controller);
  free_game_model(model);
  free_ut3k_view(view);

  return executable;
}


/** the config file is a bit of an indirection-
 * the lookup gets a list
 * foreach item in the list:
 *  lookup that list
 *  load a random sound
 */

void load_audio_from_config(config_t *cfg, char *games_directory) {
  int sound_array_setting_length, sound_samples_array_setting_length, random_sample_index;
  config_setting_t *key_sound_array, *key_sound_samples_array;
  const char *sound_type_key, *sample_name;
  char sample_filename[256];

  if (cfg == NULL) {
    return;
  }

  key_sound_array = config_lookup(cfg, CONFIG_SOUND_LIST_KEY);

  if (key_sound_array) {
    if (config_setting_is_array(key_sound_array) == CONFIG_FALSE) {
      printf("sound key array %s is not an array\n", CONFIG_SOUND_LIST_KEY);
      return;
    }

    sound_array_setting_length = config_setting_length(key_sound_array);
    printf("got array of length %d\n", sound_array_setting_length);

    for (int sound_array_index = 0; sound_array_index < sound_array_setting_length; ++sound_array_index) {
      sound_type_key = config_setting_get_string_elem(key_sound_array, sound_array_index);
      key_sound_samples_array = config_lookup(cfg, sound_type_key);
      sound_samples_array_setting_length = config_setting_length(key_sound_samples_array);
      printf("for samples %s got array of length %d\n", sound_type_key, sound_samples_array_setting_length);
      random_sample_index = rand() % sound_samples_array_setting_length;
      sample_name = config_setting_get_string_elem(key_sound_samples_array, random_sample_index);
      snprintf(sample_filename, 256, "%s%s%s", games_directory, SAMPLE_DIRNAME, sample_name);
      printf("load %s --> %s --> %s\n", sound_type_key, sample_name, sample_filename);
      ut3k_upload_wavfile(sample_filename, (char*) sound_type_key);
    }
  }
}



int main(int argc, char **argv, char **envp) {
  char *executable;
  char *games_directory, config_filename[128];
  config_t *cfg;

  cfg = (config_t*)malloc(sizeof(config_t));

  srand(time(NULL));

  config_init(cfg);

  if (! (games_directory = getenv(GAMES_LOCAL_ENV_VAR)) ) {
    games_directory = DEFAULT_GAMES_BASEDIR;
  }

  snprintf(config_filename, 128, "%s%s", games_directory, CONFIG_DIRNAME);
  config_set_include_dir(cfg, config_filename);

  strcat(config_filename, CONFIG_FILENAME);
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

  /* start audio */
  ut3k_new_audio_context();
 
  /* load audio specified from config */
  load_audio_from_config(cfg, games_directory);


  /* everything setup, run the mvc in a loop --
   * TODO: this will later be while (1) forever, but for now...
   */
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


