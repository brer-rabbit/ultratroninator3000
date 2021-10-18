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
#define UNIVERSE_Y 12
#define UNIVERSE_X 15


typedef enum { INIT, FULL_INTRO, GAME_SELECT, SHUTDOWN_PROMPT, LIFE } model_state_t;
static const uint32_t init_long_timer = 100;
static const uint32_t init_short_timer = 30;

static const char *attract_intimidate_sample = "attract_intimidate";
static const char *explosion_short_sample = "explosion_short";
static const char *effect_spin_sample = "effect_spin";
static const char *attract_coin_sample = "attract_coin";
static const char *shutdown_start_sample = "shutdown_start";
static const char *shutdown_abort_sample = "shutdown_abort";
static const char *life_sample = "life";
static const char *startup_sample = "startup";

struct game {
  char *game_executable;
  char *display_name[3];
};

struct game_model {
  struct game games[MAX_GAMES];
  int game_index;
  int num_games;
  model_state_t model_state;
  int long_timer;
  int short_timer;
  char *message_green;
  char *message_blue;
  char *message_red;
  uint8_t universe[UNIVERSE_Y][UNIVERSE_X];
  
  struct display_strategy *display_strategy;
};

static int get_games(char *directory, struct game *games_list, int max_list);
static struct display_strategy* create_display_strategy(struct game_model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 
static int evolve(void *u);
static void spawn_life(void *u);


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
  this->model_state = INIT;
  this->long_timer = init_long_timer;

  if (this->num_games == 0) {
    printf("sad panda, no games found!\n");
    free(this);
    return NULL;
  }

  ut3k_play_sample(startup_sample);

  this->display_strategy = create_display_strategy(this);
  return this;
}


void free_game_model(struct game_model *this) {
  free_display_strategy(this->display_strategy);
  return free(this);
}



void set_state_game_select(struct game_model *this) {
  this->model_state = GAME_SELECT;
}


/** next_game and prev_game
 * scroll forward/backward through the game list by 1
 */

void next_game(struct game_model *this) {
  ut3k_play_sample(explosion_short_sample);
  if (++this->game_index == this->num_games) {
    this->game_index = 0;
  }
}

void previous_game(struct game_model *this) {
  ut3k_play_sample(effect_spin_sample);
  if (--this->game_index < 0) {
    this->game_index = this->num_games - 1;
  }
}

char* get_current_executable(struct game_model *this) {
  if (this->model_state == GAME_SELECT) {
    set_blink(this, 1);
    ut3k_play_sample(attract_coin_sample);
    return this->games[this->game_index].game_executable;
  }

  return NULL;
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



void shutdown_requested(struct game_model *this) {
  if (this->model_state == GAME_SELECT) {
    this->model_state = SHUTDOWN_PROMPT;
    this->long_timer = init_long_timer;
    ut3k_play_sample(shutdown_start_sample);
  }
  else {
    this->long_timer--;
    if (this->long_timer == 0) {
      printf("shutdown!\n");
      system("sudo poweroff -i");
      this->long_timer = init_long_timer; // TODO: actually halt the system
    }
  }

  return;
}

void shutdown_aborted(struct game_model *this) {
  if (this->model_state  == SHUTDOWN_PROMPT &&
      this->long_timer < 5) {
    // easter egg! TODO...something cool
    printf("last second shutdown aborted!\n");
    spawn_life(this->universe);
    this->model_state = LIFE;
    this->short_timer = init_short_timer;
    this->long_timer = init_long_timer;
    ut3k_play_sample(life_sample);
  }
  else if (this->model_state  == SHUTDOWN_PROMPT) {
    this->model_state = GAME_SELECT;
    //ut3k_play_sample(shutdown_abort_sample);
  }
  return;
}



// make sure these strings are the same length...
static char *full_intro1 = "   ULTRATRONINATOR    ";
static char *full_intro2 = "              3000    ";
static char *full_intro3 = "                      ";

int update_full_intro(struct game_model *this) {
  if (this->model_state != FULL_INTRO) {
    this->model_state = FULL_INTRO;
    this->message_green = full_intro1;
    this->message_blue = full_intro2;
    this->message_red = full_intro3;
  }
  else {
    // scroll: advance to next letter
    this->message_green++;
    this->message_blue++;
    this->message_red++;
  }

  // return zero if the first string is at '\0'
  return this->message_green[0] == '\0' ? 0 : 1;
}




// implements f_get_display for the red display
display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;


  *blink = display_strategy->red_blink;
  *brightness = display_strategy->red_brightness;

  switch (this->model_state) {
  case GAME_SELECT:
    (*value).display_string = this->games[this->game_index].display_name[2];
    return string_display;

  case FULL_INTRO:
    (*value).display_string = this->message_red;
    return string_display;

  case SHUTDOWN_PROMPT:
    (*value).display_int = this->long_timer;
    return integer_display;

  case LIFE:
    memset(&(*value).display_glyph, 0, sizeof(uint16_t) * 4);
    for (int digit = 0; digit < 4; ++digit) {
      for (int segment = 0; segment < 15; ++segment) {
	if (this->universe[digit + 8][segment]) {
	  (*value).display_glyph[digit] |= (1 << segment);
	}
      }
    }
    return glyph_display;

  default:
    (*value).display_string = "    ";
    return string_display;
  }
}

// implements f_get_display for the blue display
display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;

  // no change
  *blink = display_strategy->blue_blink;
  *brightness = display_strategy->blue_brightness;


  switch (this->model_state) {
  case GAME_SELECT:
    (*value).display_string = this->games[this->game_index].display_name[1];
    return string_display;

  case FULL_INTRO:
    (*value).display_string = this->message_blue;
    return string_display;

  case SHUTDOWN_PROMPT:
    (*value).display_string = "DOWN";
    return string_display;

  case LIFE:
    memset(&(*value).display_glyph, 0, sizeof(uint16_t) * 4);
    for (int digit = 0; digit < 4; ++digit) {
      for (int segment = 0; segment < 15; ++segment) {
	if (this->universe[digit + 4][segment]) {
	  (*value).display_glyph[digit] |= (1 << segment);
	}
      }
    }
    return glyph_display;

  default:
    (*value).display_string = "    ";
    return string_display;
  }

}

// implements f_get_display for the green display
display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;

  // no change
  *blink = display_strategy->green_blink;
  *brightness = display_strategy->green_brightness;


  switch (this->model_state) {
  case GAME_SELECT:
    (*value).display_string = this->games[this->game_index].display_name[0];
    return string_display;

  case FULL_INTRO:
    (*value).display_string = this->message_green;
    return string_display;

  case SHUTDOWN_PROMPT:
    (*value).display_string = "SHUT";
    return string_display;

  case LIFE:
    if (--this->short_timer == 0) {
      if (evolve(this->universe) < 9) {
	// spawn new life
	spawn_life(this->universe);
      }
      this->short_timer = init_short_timer;
      if (--this->long_timer == 0) {
	this->model_state = GAME_SELECT;
      }
    }

    memset(&(*value).display_glyph, 0, sizeof(uint16_t) * 4);
    for (int digit = 0; digit < 4; ++digit) {
      for (int segment = 0; segment < 15; ++segment) {
	if (this->universe[digit][segment]) {
	  (*value).display_glyph[digit] |= (1 << segment);
	}
      }
    }
    return glyph_display;

  default:
    (*value).display_string = "    ";
    return string_display;
  }
}


static const int32_t countdown_leds[] =
  {
   0x00000000,
   0x00000001,
   0x00000003,
   0x00000007,
   0x0000000F,
   0x0000001F,
   0x0000003F,
   0x0000007F,
   0x000000FF,
   0x000001FF,
   0x000003FF,
   0x000007FF,
   0x00000FFF,
   0x00001FFF,
   0x00003FFF,
   0x00007FFF,
   0x0000FFFF,
   0x0001FFFF,
   0x0003FFFF,
   0x0007FFFF,
   0x000FFFFF,
   0x001FFFFF,
   0x003FFFFF,
   0x007FFFFF,
   0x00FFFFFF
  };


// implements f_get_display for the leds display
display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;

  // no change
  *blink = display_strategy->leds_blink;
  *brightness = display_strategy->leds_brightness;

  // I'd like to do a couple different light shows / hacks, but for now just this...
  static int led_change_timer = 0;
  static int light_show = 0x00F4A291;

  if (this->model_state == GAME_SELECT) {
    if (++led_change_timer % 15 == 0) {
      light_show = light_show << 1;
      light_show = light_show | (rand() & 0b1);
    }
  }
  else if (this->model_state == SHUTDOWN_PROMPT) {
    light_show = countdown_leds[this->long_timer >> 2];
  }
  else if (++led_change_timer % 15 == 0) {
    switch (rand() % 4) {
    case 0:
      light_show = light_show >> 1;
      light_show = light_show | (rand() & 0b100000000000);
      break;
    case 1:
      light_show = light_show << 1;
      light_show = light_show | (rand() & 0b1);
      break;
    case 2:
      light_show = ~light_show;
      break;
    case 3:
      // swap top and bottom row
      light_show = (light_show << 16) | (light_show & 0x00FF00) | ((light_show >> 16) & 0xFF);
      break;
    }
  }

    (*value).display_int = light_show;

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


/** evolve
 * life- adapted from https://rosettacode.org/
 * definitely not the most efficient approach, but it gets the job done.
 * Return is number of living items.
 */
static int evolve(void *u) {
  int life = 0;
  uint8_t (*univ)[UNIVERSE_X] = u;
  uint8_t next[UNIVERSE_Y][UNIVERSE_X];
 
  for (int y = 0; y < UNIVERSE_Y; ++y) {
    for (int x = 0; x < UNIVERSE_X; ++x) {
      int n = 0;
      for (int y1 = y - 1; y1 <= y + 1; y1++) {
	for (int x1 = x - 1; x1 <= x + 1; x1++) {
	  if (univ[(y1 + UNIVERSE_Y) % UNIVERSE_Y][(x1 + UNIVERSE_X) % UNIVERSE_X]) {
	    n++;
	  }
	}
      }
    
      if (univ[y][x]) {
	n--;
      }
      next[y][x] = (n == 3 || (n == 2 && univ[y][x]));
    }
  }

  for (int y = 0; y < UNIVERSE_Y; ++y) {
    for (int x = 0; x < UNIVERSE_X; ++x) {
      univ[y][x] = next[y][x];
      life += next[y][x];
    }
  }
  
  return life;
}

static void spawn_life(void *u) {
  uint8_t (*univ)[UNIVERSE_X] = u;
  for (int x = 0; x < UNIVERSE_X; ++x) {
    for (int y = 0; y < UNIVERSE_Y; ++y) {
      univ[y][x] = rand() < RAND_MAX / 10 ? 1 : 0;
    }
  }
}
