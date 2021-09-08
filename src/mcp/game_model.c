** initialize_game_model
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


#include "game_model.h"

#define GAMES_LOCAL_ENV_VAR "GAMES_LOCAL"
#define DEFAULT_GAMES_BASEDIR "/usr/local/games/ultratroninator_3000"

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
  char *games_directory;

  if (! (games_directory = getenv(GAMES_LOCAL_ENV_VAR)) ) {
    games_directory = DEFAULT_GAMES_BASEDIR;
  }

  this->num_games = get_games(games_directory, this->games, MAX_GAMES);
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
  if (++this->game_index == this->num_games) {
    this->game_index = 0;
  }
}

void previous_game(struct game_model *this) {
  if (--this->game_index < 0) {
    this->game_index = this->num_games - 1;
  }
}

char* get_current_executable(struct game_model *this) {
  return this->games[this->game_index].game_executable;
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
display_type get_red_display(struct display_strategy *display_strategy, display_value *value) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;
  char *red_string = get_red_string(this);
  (*value).display_string = red_string;
  return string_display;
}

// implements f_get_display for the blue display
display_type get_blue_display(struct display_strategy *display_strategy, display_value *value) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;
  char *blue_string = get_blue_string(this);
  (*value).display_string = blue_string;
  return string_display;
}

// implements f_get_display for the green display
display_type get_green_display(struct display_strategy *display_strategy, display_value *value) {
  struct game_model *this = (struct game_model*) display_strategy->userdata;
  char *green_string = get_green_string(this);
  (*value).display_string = green_string;
  return string_display;
}


struct display_strategy* get_display_strategy(struct game_model *this) {
  return this->display_strategy;
}



/* static ----------------------------------------------------------- */

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

  closedir(games_handle);

  return game_num;
}

static struct display_strategy* create_display_strategy(struct game_model *this) {
  struct display_strategy *display_strategy;
  display_strategy = (struct display_strategy*)malloc(sizeof(struct display_strategy));
  display_strategy->userdata = (void*)this;
  display_strategy->get_green_display = get_green_display;
  display_strategy->get_blue_display = get_blue_display;
  display_strategy->get_red_display = get_red_display;
  return display_strategy;
}

static void free_display_strategy(struct display_strategy *display_strategy) {
  free(display_strategy);
}
