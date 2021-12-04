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

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <libconfig.h>

#include "byte_mare.h"
#include "model.h"
#include "view_attract.h"
#include "view_map.h"
#include "view_flight.h"
#include "view_battle.h"
#include "view_gameover.h"
#include "controller_attract.h"
#include "controller_map.h"
#include "controller_flight.h"
#include "controller_battle.h"
#include "controller_gameover.h"
#include "ut3k_view.h"
#include "ut3k_pulseaudio.h"


/** byte mare
 * flight into a 1980s shareware game
 */

// configurable duration for event loop; default to 10ms
#define CONFIG_EVENT_LOOP_DURATION_TIME_KEY "event_loop_duration"
#define EVENT_LOOP_DURATION_TIME_DEFAULT 25
#define CONFIG_SOUND_LIST_KEY "sound_list"

static uint32_t clock_iterations = 0, clock_overruns = 0; // count of iterations through event loopstatic struct model *model;
static struct ut3k_view *ut3k_view;
static struct view_attract *view_attract;
static struct view_map *view_map;
static struct view_flight *view_flight;
static struct view_battle *view_battle;
static struct view_gameover *view_gameover;
static struct controller_attract *controller_attract;
static struct controller_map *controller_map;
static struct controller_flight *controller_flight;
static struct controller_battle *controller_battle;
static struct controller_gameover *controller_gameover;
static struct model *model;

void sig_cleanup_and_exit(int signum) {
  printf("caught sig %d.  Cleaning up and exiting.  Stats: %u clock ticks (%u overruns)\n",
	 signum, clock_iterations, clock_overruns);
  ut3k_remove_all_samples();
  free_view_attract(view_attract);
  free_view_map(view_map);
  free_view_flight(view_flight);
  free_view_battle(view_battle);
  free_view_gameover(view_gameover);
  free_controller_attract(controller_attract);
  free_controller_map(controller_map);
  free_controller_flight(controller_flight);
  free_controller_battle(controller_battle);
  free_controller_gameover(controller_gameover);
  free_model(model);
  free_ut3k_view(ut3k_view);
  ut3k_disconnect_audio_context();
  exit(0);
}


static void run_mvc(config_t *cfg) {
  struct timeval tval_controller_start, tval_controller_end, tval_controller_time, tval_fixed_loop_time, tval_sleep_time;
  int loop_time_ms;
  game_state_t current_game_state, next_game_state;

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

  model = create_model();
  if (model == NULL) {
    printf("error creating auto_calc model\n");
    return;
  }

  ut3k_view = create_alphanum_ut3k_view();
  if (ut3k_view == NULL) {
    printf("error creating game ut3k_view\n");
    return;
  }


  view_attract = create_view_attract(ut3k_view);
  controller_attract = create_controller_attract(model, view_attract, ut3k_view);

  view_map = create_view_map(ut3k_view);
  controller_map = create_controller_map(model, view_map, ut3k_view);

  view_flight = create_view_flight(ut3k_view);
  controller_flight = create_controller_flight(model, view_flight, ut3k_view);

  view_battle = create_view_battle(ut3k_view);
  controller_battle = create_controller_battle(model, view_battle, ut3k_view);

  view_gameover = create_view_gameover(ut3k_view);
  controller_gameover = create_controller_gameover(model, view_gameover, ut3k_view);

  if (controller_attract == NULL || controller_map == NULL ||
      controller_battle == NULL || controller_gameover == NULL ||
      controller_flight == NULL ||
      view_attract == NULL || view_map == NULL ||
      view_battle == NULL || view_gameover == NULL ||
      view_flight == NULL) {
    printf("error creating game components\n");
    return;
  }

  // only one controller can listen at a time.  Start with attract.
  register_control_panel_listener(ut3k_view, controller_attract_callback_control_panel, controller_attract);

  // init and hack:
  // start with a sleep since the view does a read from the ht16k33
  tval_sleep_time.tv_sec = 0;
  tval_sleep_time.tv_usec = tval_fixed_loop_time.tv_usec;
  
  signal(SIGINT, sig_cleanup_and_exit);
  signal(SIGTERM, sig_cleanup_and_exit);

  // event loop
  while (1) {
    // do the usleep first, otherwise the game launch is delayed by way too long

    if (tval_sleep_time.tv_sec == 0) {
      // sleep for the fixed loop time minus the time for the controller
      usleep(tval_sleep_time.tv_usec);
    }
    else {
       // this really shouldn't happen... loop takes <8 ms
      clock_overruns++;
      printf("controller took %ld.%06ld; this is longer than event loop of %ld.%06ld seconds\n", tval_controller_time.tv_sec, tval_controller_time.tv_usec, tval_fixed_loop_time.tv_sec, tval_fixed_loop_time.tv_usec);
    }

    gettimeofday(&tval_controller_start, NULL);

    current_game_state = get_game_state(model);
    switch (current_game_state) {
    case GAME_ATTRACT:
      controller_attract_update(controller_attract, clock_iterations);
      break;
    case GAME_PLAY_MAP:
      controller_map_update(controller_map, clock_iterations);
      break;
    case GAME_PLAY_FLIGHT_TUNNEL:
      controller_flight_update(controller_flight, clock_iterations);
      break;
    case GAME_PLAY_BATTLE:
      controller_battle_update(controller_battle, clock_iterations);
      break;
    case GAME_OVER:
      controller_gameover_update(controller_gameover, clock_iterations);
      break;
    default:
      printf("no controller for state\n");
    }

    next_game_state = get_game_state(model);
    if (current_game_state != next_game_state) {
      // swap out the listener based on what the next state will be
      switch (next_game_state){
      case GAME_ATTRACT:
        register_control_panel_listener(ut3k_view, controller_attract_callback_control_panel, controller_attract);
        break;
      case GAME_PLAY_MAP:
        register_control_panel_listener(ut3k_view, controller_map_callback_control_panel, controller_map);
        break;
      case GAME_PLAY_FLIGHT_TUNNEL:
        register_control_panel_listener(ut3k_view, controller_flight_callback_control_panel, controller_flight);
        controller_flight_init(controller_flight);
        break;
      case GAME_PLAY_BATTLE:
        register_control_panel_listener(ut3k_view, controller_battle_callback_control_panel, controller_battle);
        break;
      case GAME_OVER:
        register_control_panel_listener(ut3k_view, controller_gameover_callback_control_panel, controller_gameover);
        break;
      default:
        printf("no controller for state\n");
      }
    }

    ut3k_pa_mainloop_iterate();

    gettimeofday(&tval_controller_end, NULL);
    // tval_controller_time is duration of controller_update
    // tval_fixed_loop_time - tval_controller_time ==> time to sleep between iterations
    timersub(&tval_controller_end, &tval_controller_start, &tval_controller_time);
    timersub(&tval_fixed_loop_time, &tval_controller_time, &tval_sleep_time);


    clock_iterations++;

    //printf("controller time: %ld.%06ld  sleep time avail: %ld.06%ld\n", tval_controller_time.tv_sec,tval_controller_time.tv_usec, tval_sleep_time.tv_sec,tval_sleep_time.tv_usec);

  }


  ut3k_remove_all_samples();
  free_view_attract(view_attract);
  free_view_map(view_map);
  free_view_battle(view_battle);
  free_view_gameover(view_gameover);
  free_controller_attract(controller_attract);
  free_controller_map(controller_map);
  free_controller_battle(controller_battle);
  free_controller_gameover(controller_gameover);
  free_model(model);
  free_ut3k_view(ut3k_view);
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
      random_sample_index = rand() % sound_samples_array_setting_length;
      sample_name = config_setting_get_string_elem(key_sound_samples_array, random_sample_index);
      snprintf(sample_filename, 256, "%s%s%s", games_directory, SAMPLE_DIRNAME, sample_name);
      printf("load %s --> %s --> %s\n", sound_type_key, sample_name, sample_filename);
      ut3k_upload_wavfile(sample_filename, (char*) sound_type_key);
    }
  }
}



int main(int argc, char **argv, char **envp) {
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
   */
  run_mvc(cfg);

  if (cfg) {
    config_destroy(cfg);
    free(cfg);
  }


  return 0;
}


