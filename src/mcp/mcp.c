#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pigpio.h>

#include "ht16k33.h"

#include "game_model.h"
#include "game_view.h"
#include "game_controller.h"

#define GAMES_LOCAL_ENV_VAR "GAMES_LOCAL"
#define DEFAULT_GAMES_BASEDIR "/home/kaf/src/ultratroninator3000/src/mcp/"

#define MAX_GAMES 32
#define MAX_PATH 512
#define GAME_TOKEN_SEP "_"


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

typedef struct game {
  char *game_executable;
  char *display_name[3];
} *game;


int get_games(char *directory, game *games_list, int max_list);




int main(int argc, char **argv) {
  char *games_directory;
  game game_list[MAX_GAMES];
  int num_games;

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

  if (! (games_directory = getenv(GAMES_LOCAL_ENV_VAR)) ) {
    games_directory = DEFAULT_GAMES_BASEDIR;
  }

  num_games = get_games(games_directory, game_list, MAX_GAMES);

  if (num_games == 0) {
    printf("sad panda, no games found!\n");
    return 1;
  }


  // scroll through all the games
  for (int i = 0; i < num_games; ++i) {
    controller_update_data(controller, game_list[i]->display_name[0]);
    sleep(3);
  }

  free_game_view(view);

  return 0;
}


/**
 * get_games
 *
 * return a list of games and their displayable names.
 * Allocates memory - free with release_games
 */

int get_games(char *directory, game *games_list, int max_games) {
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
      //printf("  %s file is executable\n", absolute_path);
      games_list[game_num] = (game) malloc(sizeof(struct game));
      games_list[game_num]->game_executable = strndup(absolute_path, MAX_PATH);
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
	games_list[game_num]->display_name[row] =
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



