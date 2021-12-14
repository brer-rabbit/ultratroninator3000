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
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "model.h"
#include "ut3k_pulseaudio.h"


// indexed from zero, values are inclusive
static const int quadrant_max_x = 3;
static const int quadrant_max_y = 5;
//static const int sector_max_x = 7;
static const int sector_max_y = 5;
static const int sector_max_z = 7;


static const int default_player_starting_ammo = 64;
static const int default_player_starting_fuel = 64;
static const int default_player_starting_shields = 16;
static const int default_player_start_quadrant_x = quadrant_max_x;
static const int default_player_start_quadrant_y = 0;
static const int default_player_start_sector_x = 0;
static const int default_player_start_sector_y = sector_max_y / 2;
static const int default_player_start_sector_z = sector_max_z;

static const int default_motogroup_movement_timer = 250;
static const int default_motogroup_movement_timer_randomness = 100;

static const int default_flight_tunnel_movement_start_timer = 18;
static const int default_flight_tunnel_movement_end_timer = 4;
static const int default_flight_tunnel_min_width = 4;
static const int default_flight_tunnel_max_width = 6;
static const int default_flight_tunnel_max_offset_and_width = 20;

static const int default_init_battle_terrain_segment_length = 6;
static const int default_init_battle_terrain_distance_on_segment = 0;
static const int default_init_battle_terrain_elevation = 1;
static const int default_battle_terrain_x_max = 9;
static const int default_battle_terrain_y_max = 5;
static const int default_battle_terrain_z_min = 0;
static const int default_battle_terrain_z_max = 6;
static const int default_battle_terrain_segment_length_min = 5;
static const int default_battle_terrain_segment_length_max = 20;
static const int default_battle_terrain_scroll_timer = 15;
static const int terrain_z_distribution[] = { 0, 0, 1, 1, 1, 2, 2, 3, 3, 4, 5, 6 };
static const int default_battle_player_x = default_battle_terrain_x_max / 2;
static const int default_battle_player_y = 1;
static const int default_battle_player_z = default_battle_terrain_z_max;
static const int battle_max_x = 9;
static const int battle_max_y = 5;
static const int battle_max_z = 9;

static const int battle_moto_move_min_timer = 30;
static const int battle_moto_move_max_timer = 120;
static const int battle_moto_move_min_y = 3;

static struct xyz shuffled_xy_placements[] =
  { { 9,5,1 }, { 8,5,1 }, { 7,5,1 }, { 6,5,1 }, { 5,5,1 }, { 4,5,1 }, { 3,5,1 }, { 2,5,1 }, { 1,5,1 }, { 0,5,1 },
    { 9,4,1 }, { 8,4,1 }, { 7,4,1 }, { 6,4,1 }, { 5,4,1 }, { 4,4,1 }, { 3,4,1 }, { 2,4,1 }, { 1,4,1 }, { 0,4,1 },
    { 9,3,1 }, { 8,3,1 }, { 7,3,1 }, { 6,3,1 }, { 5,3,1 }, { 4,3,1 }, { 3,3,1 }, { 2,3,1 }, { 1,3,1 }, { 0,3,1 },
    { 9,2,1 }, { 8,2,1 }, { 7,2,1 }, { 6,2,1 }, { 5,2,1 }, { 4,2,1 }, { 3,2,1 }, { 2,2,1 }, { 1,2,1 }, { 0,2,1 }
  };

typedef enum { FLIGHT_CRASH_WALL, BATTLE_CRASH_GROUND } player_hit_context_t;


struct model {
  game_state_t game_state;
  struct player player;
  struct level level;
  struct flight_path flight_path;
  struct battle battle;
};


static void init_player(struct player *player);
static void player_hit(struct model *this, player_hit_context_t context);

static void init_level(struct level *level);

static void move_motogroups(struct level *level);
static struct moto_group* get_moto_group_by_quadrant(struct model *this, struct xy *quadrant);

static void init_game_state_flight_tunnel(struct model *this);
static void clocktick_flightpath(struct model *this);

static void clocktick_battle(struct model *this);
static void shuffle_placements();
static void init_battle_moto_placement(struct battle *battle);

static int get_next_battle_terrain(struct battle *battle, int current_terrain);
static void scroll_battle_terrain(struct battle *battle);
static void clocktick_battle_motos(struct battle *battle);
static int battle_xyz_collision(struct battle *battle, struct xyz *test);


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
    clocktick_flightpath(this);
  }
  else if (this->game_state == GAME_PLAY_BATTLE) {
    clocktick_battle(this);
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

  //printf("map_move_cursor: quadrant %d, %d\n", this->player.cursor_quadrant.x, this->player.cursor_quadrant.y);
}


/** set_game_state_flight_tunnel
 *
 * on the map, the player has initiated a move.  Attempt to move from
 * current position to where the cursor is.  Only move there if the
 * position is occupied by something- don't allow a move to an empty space.
 */
void set_game_state_flight_tunnel(struct model *this) {
  // check if cursor is at a valid spot for a move
  if (get_moto_group_by_quadrant(this, &this->player.cursor_quadrant) != NULL) {
    init_game_state_flight_tunnel(this);
    // set player position to cursor position --
    // seems like a fair spot in the code to do it.
    this->player.quadrant.x = this->player.cursor_quadrant.x;
    this->player.quadrant.y = this->player.cursor_quadrant.y;
    return;
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

  // else don't move player; stay in-bounds
}




void set_game_state_battle(struct model *this) {
  // init the battle struct

  // set pointer to moto_group
  this->battle.moto_group = get_moto_group_by_quadrant(this, &this->player.quadrant);
  assert(this->battle.moto_group != NULL);

  // init the motos in the group
  init_battle_moto_placement(&this->battle);
  

  this->battle.terrain_segment_length = default_init_battle_terrain_segment_length;
  this->battle.terrain_distance_on_segment = default_init_battle_terrain_distance_on_segment;
  this->battle.terrain_start_elevation = default_init_battle_terrain_elevation;
  this->battle.terrain_end_elevation = default_init_battle_terrain_elevation;
  this->battle.terrain_scroll_timer_reset = default_battle_terrain_scroll_timer;
  this->battle.terrain_scroll_timer_remaining = default_battle_terrain_scroll_timer;

  // init the landscape (terrain)
  for (int i = 0; i < TERRAIN_DISTANCE; ++i) {
    this->battle.terrain_map[i] = get_next_battle_terrain(&this->battle, default_battle_terrain_z_min);
  }

  this->player.sector.x = default_battle_player_x;
  this->player.sector.y = default_battle_player_y;
  this->player.sector.z = default_battle_player_z;
  this->game_state = GAME_PLAY_BATTLE;
}


const struct battle* get_battle(struct model *this) {
  return &this->battle;
}

void battle_move_player_x(struct model *this, int amount) {
  if (amount > 0 && this->player.sector.x < battle_max_x) {
    this->player.sector.x++;
  }
  else if (amount < 0 && this->player.sector.x > 0) {
    this->player.sector.x--;
  }
  printf("battle move player %d %d %d (x: %d)\n",
         this->player.sector.x,
         this->player.sector.y,
         this->player.sector.z,
         amount);
}
void battle_move_player_y(struct model *this, int amount) {
  if (amount > 0 && this->player.sector.y < battle_max_y) {
    this->player.sector.y++;
  }
  else if (amount < 0 && this->player.sector.y > 0) {
    this->player.sector.y--;
  }
  printf("battle move player %d %d %d (y: %d)\n",
         this->player.sector.x,
         this->player.sector.y,
         this->player.sector.z,
         amount);
}
void battle_move_player_z(struct model *this, int amount) {
  if (amount > 0 && this->player.sector.z < battle_max_z) {
    this->player.sector.z++;
  }
  else if (amount < 0 && this->player.sector.z > 0) {
    this->player.sector.z--;
  }
  printf("battle move player %d %d %d (z: %d)\n",
         this->player.sector.x,
         this->player.sector.y,
         this->player.sector.z,
         amount);
}



/* static ----------------------------------------------------------- */


static void init_player(struct player *player) {
  *player = (struct player const)
    {
     .stores = {
                .bullets = default_player_starting_ammo,
                .fuel = default_player_starting_fuel,
                .shields = default_player_starting_shields
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


static void player_hit(struct model *this, player_hit_context_t context) {
  if (--this->player.stores.shields == 0) {
    this->game_state = GAME_OVER;
  }

  switch (context) {
  case FLIGHT_CRASH_WALL:
    break;
  default:
    break;
  }
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
      for (int j = 0; j < MAX_MOTOS_PER_GROUP; ++j) {
        if (j < level->moto_groups[i].num_motos) {
          // init the moto status
          level->moto_groups[i].motos[j].status = ACTIVE;
        }
        else {
          level->moto_groups[i].motos[j].status = NOT_IN_LEVEL;
        }

      }
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



/** get_moto_group_by_quadrant
 *
 * find by xy quadrant a matching motogroup.  Return pointer to it or
 * null if nothing found.
 */
static struct moto_group* get_moto_group_by_quadrant(struct model *this, struct xy *quadrant) {
  for (int i = 0; i < this->level.num_moto_groups; ++i) {
    if (this->level.moto_groups[i].status == ACTIVE &&
        quadrant->x == this->level.moto_groups[i].quadrant.x &&
        quadrant->y == this->level.moto_groups[i].quadrant.y) {
      // found it
      return &this->level.moto_groups[i];
    }
  }
  return NULL;
}



static void flight_tunnel_serpentine(int width, int *offset, int *go_left) {
  if (*go_left) {
    if (*offset > 0) {
      (*offset)--;
    }
    else {
      *go_left = 0;
      (*offset)++;
    }
  }
  else {
    if (*offset + width < default_flight_tunnel_max_offset_and_width) {
      (*offset)++;
    }
    else {
      *go_left = 1;
      (*offset)--;
    }
  }
}


/** init_game_state_flight_tunnel
 *
 * set state to the flight tunnel thingy.  Init the data structures required.
 */
static void init_game_state_flight_tunnel(struct model *this) {
  int width, offset, i;

  // construct a tunnel

  // intro to tunnel- narrow it down
  for (width = default_flight_tunnel_max_offset_and_width, offset = 0, i = 0;
       i < 8; ++i, width = width - 2, ++offset) {
    this->flight_path.slice[i].width = width;
    this->flight_path.slice[i].offset = offset;
  }


  // do some serpentine stuff to start
  int go_left = rand() & 0b1;

  for (width = 6,
       offset = this->flight_path.slice[i - 1].offset;
       i < 50; ++i) {
    flight_tunnel_serpentine(width, &offset, &go_left);
    this->flight_path.slice[i].width = width;
    this->flight_path.slice[i].offset = offset;
  }


  for (width = this->flight_path.slice[i - 1].width,
       offset = this->flight_path.slice[i - 1].offset;
       i < 120; ++i) {
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

  // do some serpentine stuff before finishing up
  go_left = rand() & 0b1;

  for (width = 6,
       offset = this->flight_path.slice[i - 1].offset;
       i < 140; ++i) {
    flight_tunnel_serpentine(width, &offset, &go_left);
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
  this->flight_path.scroll_timer_reset = default_flight_tunnel_movement_start_timer;
  this->flight_path.scroll_timer_remaining = this->flight_path.scroll_timer_reset;

  this->player.flight_tunnel.x = default_flight_tunnel_max_offset_and_width / 2;
  this->player.flight_tunnel.y = 0;

  this->game_state = GAME_PLAY_FLIGHT_TUNNEL;
}


static void clocktick_flightpath(struct model *this) {
  if (this->flight_path.scroll_timer_remaining-- == 0) {
    // start at start_timer and linear increase to end_timer over the
    // course of the tunnel
    this->flight_path.scroll_timer_reset =
      default_flight_tunnel_movement_start_timer - (((default_flight_tunnel_movement_start_timer - default_flight_tunnel_movement_end_timer) * this->player.flight_tunnel.y) / this->flight_path.flight_path_length) - 1;

    //printf("new timer: %d\n", this->flight_path.scroll_timer_reset);
    this->flight_path.scroll_timer_remaining = this->flight_path.scroll_timer_reset;
    this->player.flight_tunnel.y++;
  }
  // check for collisions
  if (this->player.flight_tunnel.x <
      this->flight_path.slice[ this->player.flight_tunnel.y ].offset) {
    this->player.flight_tunnel.x = this->flight_path.slice[ this->player.flight_tunnel.y ].offset;
    player_hit(this, FLIGHT_CRASH_WALL);
  }
  else if (this->player.flight_tunnel.x >=
           this->flight_path.slice[ this->player.flight_tunnel.y ].offset +
           this->flight_path.slice[ this->player.flight_tunnel.y ].width) {
    // TODO: decrement shield or whatevz, play sound and all that
    this->player.flight_tunnel.x =
      this->flight_path.slice[ this->player.flight_tunnel.y ].offset +
      this->flight_path.slice[ this->player.flight_tunnel.y ].width - 1;
    player_hit(this, FLIGHT_CRASH_WALL);
  }


}


/** clocktick_battle
 *
 * objectives:
 * scroll terrain
 * TODO:
 * move bad dudes
 * move shots
 * determine next state
 */
static void clocktick_battle(struct model *this) {

  if (--this->battle.terrain_scroll_timer_remaining == 0) {
    this->battle.terrain_scroll_timer_remaining = this->battle.terrain_scroll_timer_reset;
    // scroll terrain
    scroll_battle_terrain(&this->battle);
  }

  clocktick_battle_motos(&this->battle);
}


/** shuffle_placements
 *
 * shuffle the global array `shuffled_xy_placements`
 */
static void shuffle_placements() {
  struct xyz tmp;
  for (int i = (sizeof(shuffled_xy_placements) / sizeof(struct xyz)) - 1; i > 0; --i) {
    int j = rand() % (i + 1);
    // swap i & j.. todo: switch these to pointers
    tmp.x = shuffled_xy_placements[i].x;
    tmp.y = shuffled_xy_placements[i].y;
    tmp.z = shuffled_xy_placements[i].z;

    shuffled_xy_placements[i].x = shuffled_xy_placements[j].x;
    shuffled_xy_placements[i].y = shuffled_xy_placements[j].y;
    shuffled_xy_placements[i].z = shuffled_xy_placements[j].z;

    shuffled_xy_placements[j].x = tmp.x;
    shuffled_xy_placements[j].y = tmp.y;
    shuffled_xy_placements[j].z = tmp.z;
  }
}



/** init_battle_moto_placement
 *
 * set the motos somewhere on the battle terrain.  Only one moto per
 * sector.
 * init the movement timer for motos.
 */
static void init_battle_moto_placement(struct battle *battle) {
  shuffle_placements();

  for (int i = 0; i < battle->moto_group->num_motos; ++i) {
    battle->moto_group->motos[i].sector.x = shuffled_xy_placements[i].x;
    battle->moto_group->motos[i].sector.y = shuffled_xy_placements[i].y;
    battle->moto_group->motos[i].sector.z = shuffled_xy_placements[i].z;
    printf("moto %i starting %d,%d,%d\n", i,
           battle->moto_group->motos[i].sector.x,
           battle->moto_group->motos[i].sector.y,
           battle->moto_group->motos[i].sector.z);
    battle->moto_group->motos[i].movement_timer_reset =
      (rand() % (battle_moto_move_max_timer - battle_moto_move_min_timer)) +
      battle_moto_move_min_timer;
    battle->moto_group->motos[i].movement_timer_remaining =
      battle->moto_group->motos[i].movement_timer_reset;
  }
}

/** get_next_battle_terrain
 *
 * use some whacky algorithm to construct an terrain profile that isn't
 * completely wonky yet is ragged enough to make the game interesting.
 * this will need to be completely redone and with some thought next time.
 */
static int get_next_battle_terrain(struct battle *battle, int current_terrain) {

  if (++battle->terrain_distance_on_segment == battle->terrain_segment_length) {
    // determine a new segment length, target elevation, and reset our distance on it
    battle->terrain_distance_on_segment = 0;
    battle->terrain_segment_length = rand() %
      (default_battle_terrain_segment_length_max - default_battle_terrain_segment_length_min) + default_battle_terrain_segment_length_min;

    // previous end elevation is new segment start elevation; and
    // determine a new end elevation
    battle->terrain_start_elevation = battle->terrain_end_elevation;
    battle->terrain_end_elevation = terrain_z_distribution[ rand() % (sizeof(terrain_z_distribution) / sizeof(int)) ];
    printf("landscape: length %d elevation: %d\n",
           battle->terrain_segment_length,
           battle->terrain_end_elevation);
  }


  return ((battle->terrain_end_elevation - battle->terrain_start_elevation) *
          battle->terrain_distance_on_segment) / battle->terrain_segment_length + battle->terrain_start_elevation;
}

/** scroll_battle_terrain
 *
 * copy the terrain map from 1-TERRAIN_DISTANCE down by one, dropping
 * the initial element.  Add a new element to the back.  Really ought
 * to have a simple queue for this.  It's only 6 elements though...
 * TODO: Any ground-based objects (motos) need to follow the terrain as
 * moves up or down
 */
static void scroll_battle_terrain(struct battle *battle) {
  int i;

  for (i = 0; i < TERRAIN_DISTANCE - 1; ++i) {
    battle->terrain_map[i] = battle->terrain_map[i + 1];
  }

  battle->terrain_map[i] = get_next_battle_terrain(battle, battle->terrain_map[i - 1]);

  // moto's z value is the same as terrain at it's y-value + 1
  for (int i = 0; i < battle->moto_group->num_motos; ++i) {
    battle->moto_group->motos[i].sector.z =
      battle->terrain_map[ battle->moto_group->motos[i].sector.y ] + 1;
  }
}


/** clocktick_battle_motos
 *
 * clocktick the motos in the battle to see if they should move.  If
 * timer ticks down, move the moto a random xy if the space is open.
 * Reset timer.  Adjust z to elevation.
 */

static void clocktick_battle_motos(struct battle *battle) {
  int moved = 0;
  struct xyz movement_test;


  for (int i = 0; i < battle->moto_group->num_motos; ++i) {
    if (--battle->moto_group->motos[i].movement_timer_remaining == 0) {
      // reset timer
      battle->moto_group->motos[i].movement_timer_remaining =
        battle->moto_group->motos[i].movement_timer_reset;
      // pick a random direction and try to move that way if it's legal
      switch (rand() % 4) {
      case 0:
        // y+1
        if (battle->moto_group->motos[i].sector.y < battle_max_y) {
          movement_test.x = battle->moto_group->motos[i].sector.x;
          movement_test.y = battle->moto_group->motos[i].sector.y + 1;
          movement_test.z = battle->terrain_map[ battle->moto_group->motos[i].sector.y ] + 1;
          moved = 1;
        }
        break;
      case 1:
        // y-1
        if (battle->moto_group->motos[i].sector.y > battle_moto_move_min_y) {
          movement_test.x = battle->moto_group->motos[i].sector.x;
          movement_test.y = battle->moto_group->motos[i].sector.y - 1;
          movement_test.z = battle->terrain_map[ battle->moto_group->motos[i].sector.y ] + 1;
          moved = 1;
        }
        break;
      case 2:
        // x+1
        if (battle->moto_group->motos[i].sector.x < battle_max_x) {
          movement_test.x = battle->moto_group->motos[i].sector.x + 1;
          movement_test.y = battle->moto_group->motos[i].sector.y;
          movement_test.z = battle->terrain_map[ battle->moto_group->motos[i].sector.y ] + 1;
          moved = 1;
        }
        break;
      case 3:
        // x-1
        if (battle->moto_group->motos[i].sector.x > 0) {
          movement_test.x = battle->moto_group->motos[i].sector.x - 1;
          movement_test.y = battle->moto_group->motos[i].sector.y;
          movement_test.z = battle->terrain_map[ battle->moto_group->motos[i].sector.y ] + 1;
          moved = 1;
        }
        break;
      }

      // test that the candidate location is empty
      if (moved && battle_xyz_collision(battle, &movement_test) == 0) {
        battle->moto_group->motos[i].sector.x = movement_test.x;
        battle->moto_group->motos[i].sector.y = movement_test.y;
        battle->moto_group->motos[i].sector.z = movement_test.z;
      }
    }
  }
}


/** battle_xyz_collision
 *
 * test if the struct xyz location is occupied by a moto/vehicle
 * Return zero for empty otherwise non-zero for occupied
 */
static int battle_xyz_collision(struct battle *battle, struct xyz *test) {
  for (int i = 0; i < battle->moto_group->num_motos; ++i) {
    if (battle->moto_group->motos[i].sector.x == test->x &&
        battle->moto_group->motos[i].sector.y == test->y &&
        battle->moto_group->motos[i].sector.z == test->z) {
      return 1;
    }
  }
  return 0;
}
