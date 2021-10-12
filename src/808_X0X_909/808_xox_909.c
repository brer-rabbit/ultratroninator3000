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

#include "808_xox_909.h"
#include "model.h"
#include "controller.h"
#include "ut3k_view.h"


/** 808/909 drum machine
 *
 */

// configurable duration for event loop-
// start with a default but unlike most other apps this one is
// dynamic based on the BPM
#define CONFIG_EVENT_LOOP_DURATION_TIME_KEY "event_loop_duration"
#define EVENT_LOOP_DURATION_TIME_DEFAULT 10
#define CONFIG_SOUND_LIST_KEY "sound_list"

static uint32_t clock_iterations = 0, clock_overruns = 0; // count of iterations through event loop
static struct model *model = NULL;
static struct ut3k_view *view = NULL;
static struct controller *controller = NULL;


/** bpm_128th_to_useconds
 * given a bpm, return the number of microseconds a 128th note should be.
 * 60,000 ms == 1 minute
 * 1 beat == 1/4 note
 * 128th note = 32 * 1/4 notes
 * thus (60000 / 128) / bpm ==> 1875ms / bpm ==> 1875000us / bpm
 */
static suseconds_t bpm_128th_to_useconds(int bpm) {
  return 1875000 / bpm;
}

static void run_mvc(config_t *cfg, char ***sample_keys) {
  struct timeval tval_controller_start, tval_controller_end, tval_controller_time, tval_fixed_loop_time, tval_sleep_time;
  int loop_time_ms;
  int bpm = 120;


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

  model = create_model(sample_keys);
  if (model == NULL) {
    printf("error creating auto_calc model\n");
    return;
  }

  view = create_alphanum_ut3k_view();
  if (view == NULL) {
    printf("error creating game view\n");
    return;
  }


  controller = create_controller(model, view);
  if (controller == NULL) {
    printf("error creating game controller\n");
    return;
  }

  register_control_panel_listener(view, controller_callback_control_panel, controller);

  // init and hack:
  // start with a sleep since the view does a read from the ht16k33
  tval_sleep_time.tv_sec = 0;
  tval_sleep_time.tv_usec = tval_fixed_loop_time.tv_usec;
  

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

    // should this function even exist or just rely on callback?
    // could instead just make this a loop around update_view?
    gettimeofday(&tval_controller_start, NULL);

    bpm = controller_update(controller, clock_iterations);
    tval_fixed_loop_time.tv_usec = bpm_128th_to_useconds(bpm);

    ut3k_pa_mainloop_iterate();

    gettimeofday(&tval_controller_end, NULL);
    // tval_controller_time is duration of controller_update
    // tval_fixed_loop_time - tval_controller_time ==> time to sleep between iterations
    timersub(&tval_controller_end, &tval_controller_start, &tval_controller_time);
    timersub(&tval_fixed_loop_time, &tval_controller_time, &tval_sleep_time);


    clock_iterations++;

    //printf("controller time: %ld.%06ld  sleep time avail: %ld.06%ld\n", tval_controller_time.tv_sec,tval_controller_time.tv_usec, tval_sleep_time.tv_sec,tval_sleep_time.tv_usec);

  }


  free_controller(controller);
  free_model(model);
  free_ut3k_view(view);

}


/** the config file is a bit of an indirection-
 * the lookup gets a list
 * foreach item in the list:
 *  lookup that list
 *  load the first four sounds from each key
 *
 * Return: char*** 2-d array of strings (so, 3-d array of chars...)
 * [sample_type 0:15][sample_variation 0:3][key_name 0:5]
 * caller responsible for freeing memory.  Have fun with that.
 */

char*** load_audio_from_config(config_t *cfg, char *games_directory) {
  int sound_array_setting_length, sound_samples_array_setting_length;
  config_setting_t *key_sound_array, *key_sound_samples_array;
  const char *sound_type_key, *sample_name;
  char *sound_type_key_num;
  char sample_filename[256];
  // magic numbers:
  // 15 instrument positions
  // 4 samples for each instrument
  // 4 + [1-4] + '\0' name for each sample
  char ***sound_keys;

  sound_keys = (char***)malloc(15 * sizeof(char**));
  for (int i = 0; i < 16; ++i) {
    sound_keys[i] = (char**)malloc(4 * sizeof(char*));
    for (int j = 0; j < 4; ++j) {
      sound_keys[i][j] = (char*)malloc(6 * sizeof(char));
    }
  }


  if (cfg == NULL) {
    return NULL;
  }

  key_sound_array = config_lookup(cfg, CONFIG_SOUND_LIST_KEY);

  if (key_sound_array) {
    if (config_setting_is_array(key_sound_array) == CONFIG_FALSE) {
      printf("sound key array %s is not an array\n", CONFIG_SOUND_LIST_KEY);
      return NULL;
    }

    sound_array_setting_length = config_setting_length(key_sound_array);
    printf("got array of length %d\n", sound_array_setting_length);

    assert(sound_array_setting_length < 16);

    for (int sound_array_index = 0; sound_array_index < sound_array_setting_length; ++sound_array_index) {
      sound_type_key = config_setting_get_string_elem(key_sound_array, sound_array_index);
      key_sound_samples_array = config_lookup(cfg, sound_type_key);
      sound_samples_array_setting_length = config_setting_length(key_sound_samples_array);
      printf("got samples array length of %d (max allowed is 4)\n", sound_samples_array_setting_length);
      assert(sound_samples_array_setting_length < 5);

      for (int sound_sample_array = 0; sound_sample_array < 4; ++sound_sample_array) {
	sample_name = config_setting_get_string_elem(key_sound_samples_array, sound_sample_array);
	snprintf(sample_filename, 256, "%s%s%s", games_directory, SAMPLE_DIRNAME, sample_name);
	sound_type_key_num = (char*)malloc(6 * sizeof(char));
	snprintf(sound_type_key_num, 6, "%s%d", sound_type_key, sound_sample_array);
	printf("load %s --> %s --> %s\n", sound_type_key_num, sample_name, sample_filename);
	ut3k_upload_wavfile(sample_filename, sound_type_key_num);
	strncpy(sound_keys[sound_array_index][sound_sample_array], sound_type_key_num, 6);
      }
    }
  }
  else {
    printf("failed to load sounds - config not present?\n");
    return NULL;
  }


  return sound_keys;
}



int main(int argc, char **argv, char **envp) {
  char *games_directory, config_filename[128];
  config_t *cfg;
  char ***sample_keys;

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
  sample_keys = load_audio_from_config(cfg, games_directory);


  /* everything setup, run the mvc in a loop --
   */
  run_mvc(cfg, sample_keys);

  if (cfg) {
    config_destroy(cfg);
    free(cfg);
  }


  return 0;
}


