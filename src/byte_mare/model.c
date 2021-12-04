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
static const int default_motogroup_movement_timer_randomness = 100;

static const int default_flight_tunnel_movement_timer = 5;
static const int default_flight_tunnel_min_width = 4;
static const int default_flight_tunnel_max_width = 6;
static const int default_flight_tunnel_max_offset_and_width = 20;




struct model {
  game_state_t game_state;
  struct player player;
  struct level level;
  struct flight_path flight_path;
};


static void init_player(struct player *player);
static void init_level(struct level *level);

static void move_motogroups(struct level *level);

static void set_flight_tunnel_state(struct model *this);



/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));

  *this = (struct model const) { 0 };
  this->game_state = GAME_ATTRACT;
  this->level.num_level = 1;
  this->flight_path = (struct flight_path const) { 0 };
  for (int i = 0; i < MAX_FLIGHT_PATH_LENGTH; ++i) {
    this->flight_path.slice[i].width = default_flight_tunnel_max_offset_and_width;
  }

  return this;
}


void free_model(struct model *this) {
  return free(this);
}




game_state_t clocktick_model(struct model *this) {
  if (this->game_state == GAME_PLAY_MAP) {
    move_motogroups(&this->level);
  }
  else if (this->game_state == GAME_PLAY_FLIGHT_TUNNEL) {
    if (this->flight_path.scroll_timer_remaining-- == 0) {
      this->flight_path.scroll_timer_remaining = this->flight_path.scroll_timer_reset;
      this->player.flight_tunnel.y++;
    }
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

void map_move_cursor(struct model *this, enum direction direction) {
  if (this->game_state == GAME_PLAY_MAP) {
    switch (direction) {
    case JOY_UP:
      this->player.cursor_quadrant.y =
        this->player.cursor_quadrant.y == quadrant_max_y ?
        quadrant_max_y : this->player.cursor_quadrant.y + 1;
      break;
    case JOY_DOWN:
      this->player.cursor_quadrant.y =
        this->player.cursor_quadrant.y == 0 ? 0 : this->player.cursor_quadrant.y - 1;
      break;
    case JOY_LEFT:
      this->player.cursor_quadrant.x =
        this->player.cursor_quadrant.x == 0 ? 0 : this->player.cursor_quadrant.x - 1;
      break;
    case JOY_RIGHT:
      this->player.cursor_quadrant.x =
        this->player.cursor_quadrant.x == quadrant_max_x ?
        quadrant_max_x : this->player.cursor_quadrant.x + 1;
      break;
    default:
      break;
    }
  }

  printf("map_move_cursor: quadrant %d, %d\n", this->player.cursor_quadrant.x, this->player.cursor_quadrant.y);
}


/** map_player_move
 *
 * on the map, the player has initiated a move.  Attempt to move from
 * current position to where the cursor is.  Only move there if the
 * position is occupied by something- don't allow a move to an empty space.
 */
void map_player_move(struct model *this) {
  // check if cursor is at a valid spot for a move
  for (int i = 0; i < this->level.num_moto_groups; ++i) {
    if (this->level.moto_groups[i].status == ACTIVE &&
        this->player.cursor_quadrant.x == this->level.moto_groups[i].quadrant.x &&
        this->player.cursor_quadrant.y == this->level.moto_groups[i].quadrant.y) {
      // legal move
      printf("player move to %d %d\n",
             this->player.quadrant.x, this->player.quadrant.y);
      set_flight_tunnel_state(this);
      // TODO: set game state
      return;
    }
  }

  printf("map_player_move: empty space / not allowed\n");
}


const struct moto_group* get_moto_groups(struct model *this) {
  return this->level.moto_groups;
}

const struct flight_path* get_flight_path(struct model *this) {
  return &this->flight_path;
}


int is_flight_path_complete(struct model *this) {
  if (this->player.flight_tunnel.y == this->flight_path.flight_path_length) {
    return 1;
  }
  return 0;
}

/** flight_move_player
 * move the player in flight:
 * -1 for left or 1 for right
 */
void flight_move_player(struct model *this, int position) {
  if (position < 0 && this->player.flight_tunnel.x > 0) {
    this->player.flight_tunnel.x--;
  }
  else if (position > 0 && this->player.flight_tunnel.x < default_flight_tunnel_max_offset_and_width - 1) {
    this->player.flight_tunnel.x++;
  }
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
     .cursor_quadrant = {
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


static int get_new_moto_group_movement_timer() {
  return default_motogroup_movement_timer +
    rand() % default_motogroup_movement_timer_randomness;
}

// populate the level with moto groups and such
static void init_level(struct level *level) {
  level->num_moto_groups = 3; // should be: level->num_level

  for (int i = 0; i < MAX_MOTO_GROUPS; ++i) {
    //if (i < level->num_level) {
    if (i < 4) {

      level->moto_groups[i].num_motos = 1 + rand() % 3;
      level->moto_groups[i].movement_timer_reset = get_new_moto_group_movement_timer();
      level->moto_groups[i].movement_timer_remaining = level->moto_groups[i].movement_timer_reset;
      // TODO: some smarts on placement.  This'll do for now.
      // only allow up to 3 visible from the start
      level->moto_groups[i].status = i < 3 ? ACTIVE : INACTIVE;
      switch (i % 3) {
      case 0:
        level->moto_groups[i].quadrant.x = 0;
        level->moto_groups[i].quadrant.y = quadrant_max_y;
        break;
      case 1:
        level->moto_groups[i].quadrant.x = 1;
        level->moto_groups[i].quadrant.y = quadrant_max_y;
        break;
      case 2:
        level->moto_groups[i].quadrant.x = 0;
        level->moto_groups[i].quadrant.y = quadrant_max_y - 1;
        break;
      }
    }
    else {
      level->moto_groups[i].status = NOT_IN_LEVEL;
    }
  }
}


/** check is a quadrant is occupied by an active motogroup
 * return 0 if empty of motogroup or non-zero if occupied
 */
static int is_quadrant_occupied_by_motogroup(struct level *level, struct xy *quadrant) {
  for (int i = 0; i < MAX_MOTO_GROUPS; ++i) {
    if (level->moto_groups[i].status == ACTIVE &&
        level->moto_groups[i].quadrant.x == quadrant->x &&
        level->moto_groups[i].quadrant.y == quadrant->y) {
      return 1;
    }      
  }
  return 0;
}

/** move_motogroups
 *
 * clocktick the groups, move if timer goes bing
 */
static void move_motogroups(struct level *level) {
  struct xy intended_quadrant;
  
  for (int i = 0; i < MAX_MOTO_GROUPS; ++i) {
    if (level->moto_groups[i].status == ACTIVE) {
      if (level->moto_groups[i].movement_timer_remaining-- == 0) {
        level->moto_groups[i].movement_timer_remaining =
          level->moto_groups[i].movement_timer_reset;
        // move either right or down-- if already maxed in that direction
        // then this movement turn is lost
        if (rand() & 0b1 && level->moto_groups[i].quadrant.x != quadrant_max_x) {
          intended_quadrant.x = level->moto_groups[i].quadrant.x + 1;
          intended_quadrant.y = level->moto_groups[i].quadrant.y;
          if (is_quadrant_occupied_by_motogroup(level, &intended_quadrant) == 0) {
            level->moto_groups[i].quadrant.x = intended_quadrant.x;
            level->moto_groups[i].quadrant.y = intended_quadrant.y;
          }
        }
        else if (level->moto_groups[i].quadrant.y != 0) {
          intended_quadrant.x = level->moto_groups[i].quadrant.x;
          intended_quadrant.y = level->moto_groups[i].quadrant.y - 1;
          if (is_quadrant_occupied_by_motogroup(level, &intended_quadrant) == 0) {
            level->moto_groups[i].quadrant.x = intended_quadrant.x;
            level->moto_groups[i].quadrant.y = intended_quadrant.y;
          }
        }
        printf("motogroup %d to %d %d\n", i,
               level->moto_groups[i].quadrant.x,
               level->moto_groups[i].quadrant.y);
      }
    }
  }
}

/** set_flight_tunnel_state
 *
 * set state to the flight tunnel thingy.  Init the data structures required.
 */
static void set_flight_tunnel_state(struct model *this) {
  int width, offset, i;

  // construct a tunnel

  // intro to tunnel- narrow it down
  for (width = default_flight_tunnel_max_offset_and_width, offset = 0, i = 0;
       i < 8; ++i, width = width - 2, ++offset) {
    this->flight_path.slice[i].width = width;
    this->flight_path.slice[i].offset = offset;
  }

  for (width = this->flight_path.slice[i - 1].width,
       offset = this->flight_path.slice[i - 1].offset;
       i < 150; ++i) {
    // TODO the fun stuff
    // for now just make it basically random.  This will need to be addressed later.
    switch (rand() % 8) {
    case 0:
      // increase width, if permissible, and only change offset if needed
      if (width < default_flight_tunnel_max_width &&
          width + offset < default_flight_tunnel_max_offset_and_width) {
        // can make it wider
        width++;
      }
      break;
    case 1:
      // increase width, if permissible.  Offset change.
      if (width < default_flight_tunnel_min_width &&
          offset > 0) {
        // tighten
        width++;
        offset--;
      }
      break;
    case 2:
      // decrease width, if permissible.  No offset change.
      if (width > default_flight_tunnel_min_width) {
        // tighten
        width--;
      }
      break;
    case 3:
      // decrease width, if permissible.  Change offset.
      if (width > default_flight_tunnel_min_width) {
        // tighten
        width--;
        offset++;
      }
      break;
    case 4:
    case 5:
      // offset increase -- jog right
      if (width + offset < default_flight_tunnel_max_offset_and_width) {
        offset++;
      }
      break;
    case 6:
    case 7:
      // offset decrease -- jog left
      if (offset > 0) {
        offset--;
      }
      break;
    }
    
    this->flight_path.slice[i].width = width;
    this->flight_path.slice[i].offset = offset;
  }

  // exit tunnel-- widen it out
  for (; i < 160; ++i) {
    offset = offset > 0 ? offset - 1 : offset;
    width = width < default_flight_tunnel_max_offset_and_width - 1 ?
      width + 2 : default_flight_tunnel_max_offset_and_width;
    this->flight_path.slice[i].width = width;
    this->flight_path.slice[i].offset = offset;
  }
  

  this->flight_path.flight_path_length = i;
  this->flight_path.scroll_timer_reset = default_flight_tunnel_movement_timer;
  this->flight_path.scroll_timer_remaining = this->flight_path.scroll_timer_reset;

  this->player.flight_tunnel.x = default_flight_tunnel_max_offset_and_width / 2;
  this->player.flight_tunnel.y = 0;

  this->game_state = GAME_PLAY_FLIGHT_TUNNEL;

  printf("flight path:\n");
  for (i = 0; i < this->flight_path.flight_path_length; ++i) {
    printf("  offset %d  width %d\n",
           this->flight_path.slice[i].offset,
           this->flight_path.slice[i].width);
  }

}
