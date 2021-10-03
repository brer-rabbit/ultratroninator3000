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


/* initialize_game_model
 *
 * get the backpacks up and going. Clear their mem first.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include "mcp.h"
#include "game_model.h"


#define MAX_GAMES 32
#define MAX_PATH 512
#define GAME_TOKEN_SEP "_"



struct game {
  char *game_executable;
  char *display_name[3];
};

struct game_model {
  struct game games[MAX_GAMES];
  int game_index;
  int num_games;
  struct display_strategy *display_strategy;
};

static int get_games(char *directory, struct game *games_list, int max_list);
static struct display_strategy* create_display_strategy(struct game_model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 

/** create the model.
 * read directory from env var where games are stored
 */
struct game_model* create_game_model() {
  struct game_model* this = (struct game_model*)malloc(sizeof(struct game_model));
  char *games_base_directory, games_bin_directory[128];

  if (! (games_base_directory = getenv(GAMES_LOCAL_ENV_VAR)) ) {
    games_base_directory = DEFAULT_GAMES_BASEDIR;
  }

  snprintf(games_bin_directory, 128, "%s/bin/", games_base_directory);

  // clear everything out
  for (int i = 0; i < MAX_GAMES; ++i) {
    this->games[i].game_executable = NULL;
    this->games[i].display_name[0] = NULL;
    this->games[i].display_name[1] = NULL;
    this->games[i].display_name[2] = NULL;
  }

  this->num_games = get_games(games_bin_directory, this->games, MAX_GAMES);
  this->game_index = 0;

  if (this->num_games == 0) {
    printf("sad panda, no games found!\n");
    free(this);
    return NULL;
  }

  this->display_strategy = create_display_strategy(this);
  return this;
}


void free_game_model(struct game_model *this) {
  free_display_strategy(this->display_strategy);
  return free(this);
}



/** next_game and prev_game
 * scroll forward/backward through the game list by 1
 */

void next_game(struct game_model *this) {
  //ut3k_play_sample("explosion_short");
  if (++this->game_index == this->num_games) {
    this->game_index = 0;
  }
}

void previous_game(struct game_model *this) {
  //ut3k_play_sample("effect_alarm");
  if (--this->game_index < 0) {
    this->game_index = this->num_games - 1;
  }
}

char* get_current_executable(struct game_model *this) {
  return this->games[this->game_index].game_executable;
}

void set_blink(struct game_model *this, int on) {
  if (on) {
    this->display_strategy->green_blink = HT16K33_BLINK_FAST;
    this->display_strategy->blue_blink = HT16K33_BLINK_FAST;
    this->display_strategy->red_blink = HT16K33_BLINK_FAST;
  }
  else {
    this->display_strategy->green_blink = HT16K33_BLINK_OFF;
    this->display_strategy->blue_blink = HT16K33_BLINK_OFF;
    this->display_strategy->red_blink = HT16K33_BLINK_OFF;
  }
}


int set_green_string(struct game_model *this, char *display_string) {
  this->games[this->game_index].display_name[0] = display_string;
  return 0;
}

char* get_green_string(struct game_model *this) {
  if (this) {
    return this->games[this->game_index].display_name[0];
  }
  return NULL;
}

int set_blue_string(struct game_model *this, char *display_string) {
  this->games[this->game_index].display_name[1] = display_string;
  return 0;
}

char* get_blue_string(struct game_model *this) {
  if (this) {
    return this->games[this->game_index].display_name[1];
  }
  return NULL;
}


int set_red_string(struct game_model *this, char *display_string) {
  this->games[this->game_index].display_name[2] = display_string;
  return 0;
}

char* get_red_string(struct game_model *this) {
  if (this) {
    return this->games[this->game_index].display_name[2];
  }
  return NULL;
}


// implements f_get_display for the red display
display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;
  char *red_string = get_red_string(this);
  (*value).display_string = red_string;

  // no change
  *blink = display_strategy->red_blink;
  *brightness = display_strategy->red_brightness;

  return string_display;
}

// implements f_get_display for the blue display
display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;
  char *blue_string = get_blue_string(this);
  (*value).display_string = blue_string;

  // no change
  *blink = display_strategy->blue_blink;
  *brightness = display_strategy->blue_brightness;

  return string_display;
}

// implements f_get_display for the green display
display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;
  char *green_string = get_green_string(this);
  (*value).display_string = green_string;

  // no change
  *blink = display_strategy->green_blink;
  *brightness = display_strategy->green_brightness;

  return string_display;
}

// DELETE ME
static int foo = 0;
static int cur = 0x00040201;

// implements f_get_display for the leds display
display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;

  if (++foo % 15 == 0) {
    cur = cur << 1;
    cur = cur | (rand() & 0b1);
  }
  
  (*value).display_int = cur;

  // no change
  *blink = display_strategy->leds_blink;
  *brightness = display_strategy->leds_brightness;

  return integer_display;
}



struct display_strategy* get_display_strategy(struct game_model *this) {
  return this->display_strategy;
}



/* static ----------------------------------------------------------- */

// qsort comparison function
static int game_comp(const void *a, const void *b) {
  struct game *game_a = (struct game*) a;
  struct game *game_b = (struct game*) b;
  return strcmp(game_a->game_executable, game_b->game_executable);
}

/**
 * get_games
 *
 * return a list of games and their displayable names.
 * Allocates memory - free with release_games
 */

static int get_games(char *directory, struct game *games_list, int max_games) {
  DIR *games_handle;
  struct dirent *directory_entry;
  struct stat stat_buffer;
  char absolute_path[MAX_PATH], *base_name;
  char *slash = "/";
  int game_num = 0;
  const int display_rows = 3, display_cols = 16;
  
  games_handle = opendir(directory);

  while ( (directory_entry = readdir(games_handle)) ) {
    if (directory_entry->d_name[0] == '.') {
      continue; // skip "." entires
    }

    base_name = strndup(directory_entry->d_name, display_rows * display_cols);
    snprintf(absolute_path, MAX_PATH, "%s%s%s", directory, slash, base_name);

    if (stat(absolute_path, &stat_buffer) == 0 && stat_buffer.st_mode & S_IXOTH) {
      //games_list[game_num] = (game) malloc(sizeof(struct game));
      games_list[game_num].game_executable = strndup(absolute_path, MAX_PATH);
      /* parse the filename to get something to display.
       * files should be named "abcd_mno_xyz" where the
       * underscores sep the parts of the name.  Store this
       * parsed representation.  Something like:
       *  abcd
       *  mno
       *  xyz
       * later feature: support glyphs, spaces
       * perl equivalent: split(/_/, game_executable, 3);
       */
      for (int row = 0; row < display_rows && base_name != NULL; ++row) {
	games_list[game_num].display_name[row] =
	  strsep(&base_name, GAME_TOKEN_SEP);
      }

      // reached the max number of games we want to track
      if (++game_num == MAX_GAMES) {
	break;
      }
    }

  }

  qsort(games_list, game_num, sizeof(struct game), game_comp);
  closedir(games_handle);

  return game_num;
}

static struct display_strategy* create_display_strategy(struct game_model *this) {
  struct display_strategy *display_strategy;
  display_strategy = (struct display_strategy*)malloc(sizeof(struct display_strategy));
  display_strategy->userdata = (void*)this;
  display_strategy->get_green_display = get_green_display;
  display_strategy->green_blink = HT16K33_BLINK_OFF;
  display_strategy->green_brightness = HT16K33_BRIGHTNESS_7;
  display_strategy->get_blue_display = get_blue_display;
  display_strategy->blue_blink = HT16K33_BLINK_OFF;
  display_strategy->blue_brightness = HT16K33_BRIGHTNESS_7;
  display_strategy->get_red_display = get_red_display;
  display_strategy->red_blink = HT16K33_BLINK_OFF;
  display_strategy->red_brightness = HT16K33_BRIGHTNESS_7;
  display_strategy->get_leds_display = get_leds_display;
  display_strategy->leds_blink = HT16K33_BLINK_OFF;
  display_strategy->leds_brightness = HT16K33_BRIGHTNESS_15;
  return display_strategy;
}

static void free_display_strategy(struct display_strategy *display_strategy) {
  free(display_strategy);
}
