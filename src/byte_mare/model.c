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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "model.h"
//#include "view_attract.h"
#include "ut3k_pulseaudio.h"


static const int quadrant_max_x = 4;
static const int quadrant_max_y = 6;
static const int sector_max_x = 8;
static const int sector_max_y = 6;
static const int sector_max_z = 8;


static const int default_player_starting_ammo = 64;
static const int default_player_start_quadrant_x = quadrant_max_x;
static const int default_player_start_quadrant_y = quadrant_max_y;
static const int default_player_start_sector_x = 0;
static const int default_player_start_sector_y = sector_max_y / 2;
static const int default_player_start_sector_z = sector_max_z;



struct model {
  game_state_t game_state;
  struct player player;
  struct level level;
};


static void init_player(struct player *player);
static void init_level(struct level *level);




/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));

  *this = (struct model const) { 0 };
  this->game_state = GAME_ATTRACT;
  this->level.num_level = 1;

  return this;
}


void free_model(struct model *this) {
  return free(this);
}




game_state_t clocktick_model(struct model *this) {
  return this->game_state;
}


game_state_t get_game_state(struct model *this) {
  return this->game_state;
}

void set_game_start(struct model *this) {
  this->game_state = GAME_PLAY_MAP;
  // set player stuff
  init_player(&this->player);
  init_level(&this->level);
  return;
}


void move_player(struct model *this, enum direction direction) {
  if (this->game_state == GAME_PLAY_MAP) {
    switch (direction) {
    case JOY_UP:
      this->player.quadrant.y =
        this->player.quadrant.y == quadrant_max_y ?
        quadrant_max_y : this->player.quadrant.y + 1;
      break;
    case JOY_DOWN:
      this->player.quadrant.y =
        this->player.quadrant.y == 0 ? 0 : this->player.quadrant.y - 1;
      break;
    case JOY_LEFT:
      this->player.quadrant.x =
        this->player.quadrant.x == 0 ? 0 : this->player.quadrant.x - 1;
      break;
    case JOY_RIGHT:
      this->player.quadrant.x =
        this->player.quadrant.x == quadrant_max_x ?
        quadrant_max_x : this->player.quadrant.x + 1;
      break;
    default:
      break;
    }
  }

  printf("move_player: quadrant %d, %d\n", this->player.quadrant.x, this->player.quadrant.y);
}


/* static ----------------------------------------------------------- */


static void init_player(struct player *player) {
  *player = (struct player const)
    {
     .ammo = {
              .bullets = default_player_starting_ammo
              },
     .quadrant = {
                  .x = default_player_start_quadrant_x,
                  .y = default_player_start_quadrant_y
                 },
     .sector = {
                .x = default_player_start_sector_x,
                .y = default_player_start_sector_y,
                .z = default_player_start_sector_z
                }
    };
}


static void init_level(struct level *level) {
}
