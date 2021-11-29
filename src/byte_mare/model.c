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


// indexed from zero, values are inclusive
static const int quadrant_max_x = 3;
static const int quadrant_max_y = 5;
static const int sector_max_x = 7;
static const int sector_max_y = 5;
static const int sector_max_z = 7;


static const int default_player_starting_ammo = 64;
static const int default_player_start_quadrant_x = quadrant_max_x;
static const int default_player_start_quadrant_y = 0;
static const int default_player_start_sector_x = 0;
static const int default_player_start_sector_y = sector_max_y / 2;
static const int default_player_start_sector_z = sector_max_z;

static const int default_motogroup_movement_timer = 250;





struct model {
  game_state_t game_state;
  struct player player;
  struct level level;
};


static void init_player(struct player *player);
static void init_level(struct level *level);

static void move_motogroups(struct level *level);




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
  if (this->game_state == GAME_PLAY_MAP) {
    move_motogroups(&this->level);
  }

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


const struct player* get_player(struct model *this) {
  return &this->player;
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


const struct moto_group* get_moto_groups(struct model *this) {
  return this->level.moto_groups;
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


// populate the level with moto groups and such

static void init_level(struct level *level) {
  for (int i = 0; i < MAX_MOTO_GROUPS; ++i) {
    if (i < level->num_level) {
      // TODO: some smarts on placement
      level->moto_groups[i].status = ACTIVE;
      level->moto_groups[i].quadrant.x = 0;
      level->moto_groups[i].quadrant.y = quadrant_max_y;
      level->moto_groups[i].num_motos = 1 + rand() % 3;
      level->moto_groups[i].movement_timer = default_motogroup_movement_timer;
      level->moto_groups[i].movement_timer_remaining = default_motogroup_movement_timer;
    }
    else {
      level->moto_groups[i].status = NOT_IN_LEVEL;
    }
  }
}


/** move_motogroups
 *
 * clocktick the groups, move if timer goes bing
 */
static void move_motogroups(struct level *level) {
  for (int i = 0; i < MAX_MOTO_GROUPS; ++i) {
    if (level->moto_groups[i].status == ACTIVE) {
      if (level->moto_groups[i].movement_timer_remaining-- == 0) {
        level->moto_groups[i].movement_timer_remaining =
          level->moto_groups[i].movement_timer;
        // move either right or down-- if already maxed in that direction
        // then this movement turn is lost
        if (rand() & 0b1) {
          level->moto_groups[i].quadrant.x =
            level->moto_groups[i].quadrant.x == quadrant_max_x ?
            quadrant_max_x : level->moto_groups[i].quadrant.x + 1;
        }
        else {
          level->moto_groups[i].quadrant.y =
            level->moto_groups[i].quadrant.y == 0 ?
            0 : level->moto_groups[i].quadrant.y - 1;
        }
        printf("motogroup %d to %d %d\n", i,
               level->moto_groups[i].quadrant.x,
               level->moto_groups[i].quadrant.y);
      }
    }
  }
}
