/** initialize_game_model
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
// prod: /usr/local/games/hacks
#define DEFAULT_GAMES_BASEDIR "/home/kaf/src/ultratroninator3000/src/mcp/"

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
};

static int get_games(char *directory, struct game *games_list, int max_list);
  

/** create the model.
 * read directory from env var where games are stored
 */
struct game_model* create_game_model() {
  struct game_model* game_model = (struct game_model*)malloc(sizeof(struct game_model));
  char *games_directory;

  if (! (games_directory = getenv(GAMES_LOCAL_ENV_VAR)) ) {
    games_directory = DEFAULT_GAMES_BASEDIR;
  }

  game_model->num_games = get_games(games_directory, game_model->games, MAX_GAMES);
  game_model->game_index = 0;

  if (game_model->num_games == 0) {
    printf("sad panda, no games found!\n");
    free(game_model);
    return NULL;
  }


  return game_model;
}

void free_game_model(struct game_model *model) {
  return free(model);
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

