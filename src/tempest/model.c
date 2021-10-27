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


/* initialize_model
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "tempest.h"
#include "model.h"
#include "view_gameplay.h"
#include "view_player_hit.h"
#include "view_levelup.h"
#include "ut3k_pulseaudio.h"


typedef enum
{
  GAME_ATTRACT,
  GAME_PLAY,
  GAME_PLAYER_HIT,
  GAME_LEVEL_UP,
  GAME_OVER
} game_state_t;
  

// state and bizrules on how to handle player getting hit



static const int default_flipper_move_timer = 20;
static const int default_flipper_flip_timer = 5;
static const int default_next_depth_timer = 60;
static const int default_player_blaster_move_timer = 6;
static const int default_player_blaster_ready_timer = 40;

static const int default_player_zapper_charging_timer = 10;
static const int default_player_zapper_firing_timer = 2;
static const int default_player_zapper_max_charge = 14;

static const int default_next_flipper_spawn_timer_randmoness = 200;
static const int default_next_flipper_spawn_timer_minumum = 50;
static const int default_max_spawning_flippers = 3;

static const int default_flash_screen_timer = 200;
static const int default_lives_remaining_timer = 1;

static const int default_text_scroll_timer = 22;

static const int default_display_iterations_slow_med = 6;
static const int default_display_iterations_fast = 18;
static const int default_display_slow_time = 17;
static const int default_display_medium_time = 10;
static const int default_display_fast_time = 5;



struct model {
  struct display_strategy *gameplay_display_strategy;
  struct display_strategy *playerhit_display_strategy;
  struct display_strategy *levelup_display_strategy;
  struct player player;
  struct playfield playfield;

  struct player_hit_and_restart player_hit_and_restart;
  struct levelup levelup;

  game_state_t game_state;
};



// model methods
static game_state_t clocktick_gameplay(struct model *this, uint32_t clock);
static void init_player_hit(struct player_hit_and_restart *this, int lives_remaining);
static void clocktick_player_hit(struct model *this, uint32_t clock);
static game_state_t clocktick_levelup(struct model *this);
static int move_flippers(struct model *this);
static int move_blasters(struct model *this);
static void update_zapper(struct model *this);
static game_state_t collision_check(struct model *this);
static void reset_player_and_nasties(struct model *this, int full);
static void spawn_flipper(struct model *this, int index);
static void init_player_for_game(struct player *player);
static void init_playfield_for_level(struct playfield *this, int level);
static void set_model_state_levelup(struct model *this);
static inline int index_on_depth(int from, int sizeof_from, int sizeof_to);


/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));
  this->gameplay_display_strategy = create_gameplay_display_strategy(this);
  this->playerhit_display_strategy = create_playerhit_display_strategy(this);
  this->levelup_display_strategy = create_levelup_display_strategy(this);

  init_player_for_game(&this->player);
  this->player_hit_and_restart = (struct player_hit_and_restart const) { 0 };
  this->levelup = (struct levelup const) { 0 };

  // just for now...get things going
  this->game_state = GAME_PLAY;
  init_playfield_for_level(&this->playfield, 1);

  return this;
}


void free_model(struct model *this) {
  free_gameplay_display_strategy(this->gameplay_display_strategy);
  free_playerhit_display_strategy(this->playerhit_display_strategy);
  free_levelup_display_strategy(this->levelup_display_strategy);
  return free(this);
}


/** clocktick_model
 *
 * tick the clock, updating based on current game state.  If game_play
 * had a collision, do not immediately change state (since that would
 * clear the board {err, model}).  Leave board up momentarily to show the
 * player what happened, then change state.
 */
void clocktick_model(struct model *this, uint32_t clock) {
  switch(this->game_state) {
  case GAME_PLAY:
    if (this->playfield.has_collision == 0) {
      if (clocktick_gameplay(this, clock) == GAME_LEVEL_UP) {
        // odd: clocktick_gameplay returns what it suggests as the next
        // state.  It's completely ignored unless it's GAME_LEVEL_UP.
        // Next state for losing a life or game over is more determined
        // by the playfield's has_collision flag getting set and a timer
        // expiring.
        set_model_state_levelup(this);
        ut3k_play_sample(level_up_soundkey);
      }
    }
    else if (--this->player_hit_and_restart.timer == 0) {
      // player was hit on a previous cycle, transition to
      // GAME_PLAYER_HIT after a delay
      if (--this->player.lives_remaining == 0) {
        printf("game over\n");
        this->game_state = GAME_OVER;        
      }
      else {
        this->game_state = GAME_PLAYER_HIT;
        init_player_hit(&this->player_hit_and_restart, this->player.lives_remaining);
      }
    }
    break;
  case GAME_PLAYER_HIT:
    // show lives left
    if (this->player_hit_and_restart.timer == 0) {
      // reset player and any remaining nasties
      reset_player_and_nasties(this, 0);
      // transition to next state
      this->game_state = GAME_PLAY;
    }
    else {
      clocktick_player_hit(this, clock);
    }
    
    break;
  case GAME_LEVEL_UP:
    // clocktick will run the between level stuff and signal
    // back with GAME_PLAY when complete
    this->game_state = clocktick_levelup(this);

    // init_levelup_display gets set on state transition to GAME_LEVEL_UP;
    // set it back to zero... this is awful since it'll get done on every
    // iteration, but, whatevz...
    this->levelup.init_levelup_display = 0;
    break;

  default:
    break;
  }
}




/** move_player
 *
 * player circles the arena at depth zero.  If the playfield allows wrap,
 * do that.  Otherwise held at the array boundaries.
 */
void move_player(struct model *this, int8_t direction) {

  if (this->playfield.has_collision == 1) {
    return;
  }

  if (direction > 0) {
    this->player.position++;
    if (this->player.position >= this->playfield.size_per_array[0]) {
      this->player.position = this->playfield.circular ? 0 : this->playfield.size_per_array[0] - 1;
    }
  }
  else if (direction < 0) {
    this->player.position--;
    if (this->player.position < 0) {
      this->player.position = this->playfield.circular ?
	this->playfield.size_per_array[0] - 1 : 0;
    }
  }

  printf("player: %d\n", this->player.position);
  ut3k_play_sample(player_move_soundkey);
}


/** set_player_blaster_fired
 *
 * fire the thingymajig.  Conditions are the blaster timer is non-positive
 * and shots are available.  Firing set a blaster shot to fired at player
 * position and reset of the blaster timer.
 */
void set_player_blaster_fired(struct model *this) {
  if (this->game_state != GAME_PLAY || this->playfield.has_collision == 1) {
    return;
  }

  if (this->player.blaster_ready_timer <= 0) {
    // timer says it's available, see if any shots are ready
    struct blaster *blaster;
    for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
      blaster = &this->player.blaster[i];
      if (blaster->blaster_state == BLASTER_READY) {
        // got one
        *blaster = (struct blaster const)
          {
           .blaster_state = BLASTER_FIRED,
           .position = this->player.position,
           .depth = this->player.depth,
           .move_timer = default_player_blaster_move_timer
          };

        if (i == 0) {
          ut3k_play_sample(player_multishot_soundkey);
        }
        else {
          ut3k_play_sample(player_shoot_soundkey);
        }
        this->player.blaster_ready_timer = default_player_blaster_ready_timer;
        return;
      }
    }
  }
}



/** set_player_zapper_charging
 *
 * if the player hasn't used the super zapper this turn, start charging
 * it
 */
void set_player_zapper(struct model *this) {
  if (this->game_state != GAME_PLAY || this->playfield.has_collision == 1) {
    return;
  }

  if (this->player.superzapper.superzapper_state == ZAPPER_READY) {
    this->player.superzapper.superzapper_state = ZAPPER_CHARGING;
    this->player.superzapper.range = 1;
    printf("charging ZAPPER!\n");
    // note: sound is queued from the update zapper based on charge level
  }
  else if (this->player.superzapper.superzapper_state == ZAPPER_CHARGING) {
    this->player.superzapper.superzapper_state = ZAPPER_FIRING;
    this->player.superzapper.position = this->player.position;
    this->player.superzapper.depth = this->player.depth;
    this->player.superzapper.timer = default_player_zapper_charging_timer;
    memset(this->player.superzapper.zapped_squares,
           0,
           sizeof(this->player.superzapper.zapped_squares));
    printf("firing ZAPPER!\n");
    ut3k_play_sample(superzapper_soundkey);
  }

}



struct display_strategy* get_display_strategy(struct model *this) {
  switch (this->game_state) {
  case GAME_PLAY:
    return this->gameplay_display_strategy;
  case GAME_PLAYER_HIT:
    return this->playerhit_display_strategy;
  case GAME_LEVEL_UP:
    return this->levelup_display_strategy;
  default:
    return this->gameplay_display_strategy;
  }    
}


const struct player* get_model_player(struct model *this) {
  return &this->player;
}

const struct playfield* get_model_playfield(struct model *this) {
  return &this->playfield;
}

const struct player_hit_and_restart* get_model_player_hit_and_restart(struct model *this) {
  return &this->player_hit_and_restart;
}

const struct levelup* get_model_levelup(struct model *this) {
  return &this->levelup;
}





/* static ----------------------------------------------------------- */


static game_state_t clocktick_gameplay(struct model *this, uint32_t clock) {
  game_state_t next_game_state;
  // move the nasties, decrement clock, make adjustments
  move_flippers(this);
  move_blasters(this);
  update_zapper(this);
  next_game_state = collision_check(this);
  
  // unchecked decrement blaster timer.
  // Non-positive is ready this will roll/error from negative to
  // positive in ~1 year.  That's acceptable.
  this->player.blaster_ready_timer--;
  return next_game_state;
}


static char *lives_remaining_string = "   ZAPPERS REMAINING:";

static void init_player_hit(struct player_hit_and_restart *this, int lives_remaining) {
  this->timer = default_lives_remaining_timer;
  this->scroll_timer = default_text_scroll_timer;
  snprintf(this->messaging, 48, "%s %d   ", lives_remaining_string, lives_remaining);
  this->msg_ptr = this->messaging;
}


static void clocktick_player_hit(struct model *this, uint32_t clock) {
  if (--this->player_hit_and_restart.scroll_timer == 0) {
    this->player_hit_and_restart.scroll_timer = default_text_scroll_timer;
    this->player_hit_and_restart.msg_ptr++;
    if (*this->player_hit_and_restart.msg_ptr == '\0') {
      // signal done scrolling
      this->player_hit_and_restart.timer = 0;
    }
  }
}


/** clocktick_levelup
 *
 * set the state so the view can query the next frame...
 * this is starting to look tightly coupled between view and model.
 * Ideally, I think we'd just want to tell the view "play the
 * animation thing" and it'd message back when it completes.
 * as-is, the model is driving which frames to play in the view.
 * Hey, I don't get paid for this, so suck it.
 */
static game_state_t clocktick_levelup(struct model *this) {
  if (--this->levelup.scroll_timer == 0) {
    if (*this->levelup.msg_ptr != '\0') {
      this->levelup.scroll_timer = default_text_scroll_timer;
      this->levelup.msg_ptr++;
    }
  }

  if (--this->levelup.animation_timer == 0) {
    if (++this->levelup.animation_iterations ==
        *this->levelup.animation_iterations_max) {
      // transition to next state
      this->levelup.animation_iterations = 0;
      if (this->levelup.animation_state == DIAG_FALLING_SLOW) {
        // display over, signal transition to next state
        return GAME_PLAY;
      }
      else {
        this->levelup.animation_state++;
      }
    }
    switch (this->levelup.animation_state) {
    case DIAG_RISING_SLOW:
    case DIAG_FALLING_SLOW:
      this->levelup.animation_timer = default_display_slow_time;
      this->levelup.animation_iterations_max = &default_display_iterations_slow_med;
      break;
    case DIAG_RISING_MED:
    case DIAG_FALLING_MED:
      this->levelup.animation_timer = default_display_medium_time;
      this->levelup.animation_iterations_max = &default_display_iterations_slow_med;
      break;
    case DIAG_RISING_FAST:
    case DIAG_FALLING_FAST:
    case FLATLINE:
      this->levelup.animation_timer = default_display_fast_time;
      this->levelup.animation_iterations_max = &default_display_iterations_fast;
      break;
    }
  }

  return GAME_LEVEL_UP;
}



/** move_flippers
 *
 * 1 move any active flippers
 * 2 move spawning flippers in the pool, advance to active if clockticks down
 * 3 promote inactive to spawning if there is room in the pool
 * return true if something moved that requires collision detection,
 * zero for nothing changed that would require collision detection.
 */
static int move_flippers(struct model *this) {
  int something_moved = 0;
  int num_flippers_spawning = 0;
  int index_inactive_flipper = -1; // pick one, any one inactive
  struct flipper *flipper;


  for (int i = 0; i < this->playfield.num_flippers; ++i) {
    flipper = &this->playfield.flippers[i];

    switch (flipper->flipper_state) {
    case INACTIVE:
      index_inactive_flipper = i;
      break;
    case SPAWNING:
      num_flippers_spawning++;
      if (--flipper->next_depth_timer == 0) {
        something_moved = 1;
        flipper->flipper_state = ACTIVE;
        flipper->depth = this->playfield.num_arrays - 1;
        flipper->position = rand() %
          this->playfield.size_per_array[ this->playfield.num_arrays - 1 ];
        flipper->next_depth_timer = default_next_depth_timer;
        flipper->intra_depth_timer = default_flipper_move_timer;
      }
      else {
        // make it move around a bit- then as the next depth timer
        // runs down launch from position 0-->7
        flipper->position = -1 * ( 8 * flipper->next_depth_timer / default_next_depth_timer - 7);
      }
      break;
    case ACTIVE:

      if (flipper->depth > 0 &&
          --flipper->next_depth_timer == 0) {
        something_moved = 1;
        flipper->next_depth_timer = default_next_depth_timer;
        flipper->depth--;
        //printf("flipper %d promoted to depth %d from position %d ", i, flipper->depth, flipper->position);
        flipper->intra_depth_timer = default_flipper_move_timer;

        flipper->position =
          index_on_depth(flipper->position,
                         this->playfield.size_per_array[flipper->depth + 1],
                         this->playfield.size_per_array[flipper->depth]);
        //printf("to position %d\n", flipper->position);
          
      }
      else if (--flipper->intra_depth_timer == 0) {
        // reset timer to the flipper flip timer
        something_moved = 1;
        flipper->intra_depth_timer = default_flipper_flip_timer;
        // we're ACTIVE and our timer expired, start transition out of
        // this position
        flipper->flipper_state = FLIPPING_FROM1;

      }
      break;

    case FLIPPING_FROM3:
      // on this first clock of the state,
      // actually move to the next position, checking for boundary conditions
      if (flipper->intra_depth_timer == default_flipper_flip_timer) {
        something_moved = 1;
        flipper->position +=
          flipper->movement_direction;
        if (flipper->position >= this->playfield.size_per_array[flipper->depth]) {
          // don't flip past the array end
          flipper->position = 0;
        }
        else if (flipper->position < 0) {
          flipper->position =
            this->playfield.size_per_array[flipper->depth] - 1;
        }
        //printf("flipper %d moved to position %d depth %d\n", i, flipper->position, flipper->depth);
      }

    // fallthrough
    case FLIPPING_FROM1:
    case FLIPPING_FROM2:
    case FLIPPING_TO3:
    case FLIPPING_TO2:
      if (--flipper->intra_depth_timer == 0) {
        something_moved = 1;
        flipper->intra_depth_timer = default_flipper_flip_timer;
        flipper->flipper_state++;
      }
      break;

    case FLIPPING_TO1:
      if (--flipper->intra_depth_timer == 0) {
        something_moved = 1;
        flipper->intra_depth_timer = default_flipper_move_timer;
        flipper->flipper_state = ACTIVE;
      }
      break;

    case DESTROYED:
    default:
      break;
    } // switch

  } // for

  // if nothing is spawning, setup an inactive flipper
  if (--this->playfield.next_flipper_spawn_timer == 0) {
    this->playfield.next_flipper_spawn_timer = rand() % default_next_flipper_spawn_timer_randmoness + default_next_flipper_spawn_timer_minumum;
    if (num_flippers_spawning < default_max_spawning_flippers &&
        index_inactive_flipper != -1) {
      //printf("spawning flipper index %d\n", index_inactive_flipper);
      spawn_flipper(this, index_inactive_flipper);
    }
  }

  return something_moved;
}


/** move_blasters
 *
 * move the blaster shots according to their timer.  Return true
 * if something actually moved; zero for nothing changed.
 */
static int move_blasters(struct model *this) {
  int something_moved = 0;
  struct blaster *blaster;

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    blaster = &this->player.blaster[i];
    if (blaster->blaster_state == BLASTER_FIRED) {
      if (--blaster->move_timer == 0) {
        something_moved = 1;
        blaster->move_timer = default_player_blaster_move_timer;
        if (blaster->depth == this->playfield.num_arrays - 1) {
          // blaster shot is at the max depth, reset back to ready
          blaster->blaster_state = BLASTER_READY;
        }
        else {
          // move the shot
          printf("blaster %d moved from %d depth %d ",
                 i, blaster->position, blaster->depth);
          blaster->position =
            index_on_depth(blaster->position,
                           this->playfield.size_per_array[blaster->depth],
                           this->playfield.size_per_array[blaster->depth + 1]);
          blaster->depth++;
          printf("to %d depth %d\n",
                 blaster->position, blaster->depth);

        }
      }
    }
  }
  return something_moved;
}

/** update_zapper
 *
 * if the player is charging the zapper, increment it up.
 * if the zapper is fired, zap the necessary adjacent spaces
 */
static void update_zapper(struct model *this) {
  struct superzapper *superzapper = &this->player.superzapper;
  if (superzapper->superzapper_state == ZAPPER_CHARGING) {
    if (--superzapper->timer == 0) {
      superzapper->timer = default_player_zapper_charging_timer;
      superzapper->range++;
      switch (superzapper->range) {
        // these are somewhat arbitrary-- give the player a sense
        // the thing is powering up
      case 2:
        ut3k_play_sample(electric1_soundkey);
        break;
      case 3:
        ut3k_play_sample(electric2_soundkey);
        break;
      case 5:
        ut3k_play_sample(electric3_soundkey);
        break;
      case 7:
        ut3k_play_sample(electric4_soundkey);
        break;
      case 9:
        ut3k_play_sample(electric5_soundkey);
        break;
      case 11:
        ut3k_play_sample(electric6_soundkey);
        break;
      case 13:
        ut3k_play_sample(electric7_soundkey);
        break;
      default:
        break;
      }
    }
  }
  else if (superzapper->superzapper_state == ZAPPER_FIRING) {
    if (--superzapper->timer == 0) {
      superzapper->timer = default_player_zapper_firing_timer;

      if (++superzapper->zapped_range > superzapper->range) {
        // that's as far as it goes, reset
        superzapper->superzapper_state = ZAPPER_DEPLETED;
        superzapper->zapped_range = 0;
        superzapper->range = 0;
      }
      else { // calculate the square blown away
        // anything on the same depth as the superzapper may be blown away.
        // create map for lookup- thinking this may be the easiest way
        // to check an index in what might be a circular buffer.
        // range is the potential that it'll get to
        // zapped_range is the range hit so far
        if (this->playfield.circular == 1) {
          for (int i = this->player.superzapper.position - this->player.superzapper.zapped_range;
               i <= this->player.superzapper.position + this->player.superzapper.zapped_range;
               ++i) {
            if (i < 0) {
              this->player.superzapper.zapped_squares[this->playfield.size_per_array[0] + i] = 1;
            }
            else {
              this->player.superzapper.zapped_squares[i % this->playfield.size_per_array[0]] = 1;
            }
          }
        }
        else { // non-circular: don't wrap the superzapper spaces
          for (int i = this->player.superzapper.position - this->player.superzapper.zapped_range;
               i <= this->player.superzapper.position + this->player.superzapper.zapped_range;
               ++i) {
            if (i >= 0 && i < this->playfield.size_per_array[0]) {
              this->player.superzapper.zapped_squares[i] = 1;
            }
          }
        }
      }
    }
  }
}


/** collision_check
 *
 * pre: everything has moved for this clocktick.
 * check if any blaster shots occupy the position of a nasty.
 * Check if any nasty is in the same space as the player.
 * Check if player overcharged the superzapper.
 * May result in a (suggested) state change for the model
 */
static game_state_t collision_check(struct model *this) {
  struct flipper *flipper;
  struct blaster *blaster;
  int flippers_remaining = this->playfield.num_flippers;


  // plain vanilla O(N*N) approach to this
  for (int i = 0; i < this->playfield.num_flippers; ++i) {
    flipper = &this->playfield.flippers[i];
    if (flipper->flipper_state == DESTROYED) {
      flippers_remaining--;
      continue;
    }
    else if (flipper->flipper_state == INACTIVE ||
             flipper->flipper_state == SPAWNING) {
      continue; // ignore this flipper
    }

    // blaster can destroy an active or flipping nasty
    for (int j = 0; j < MAX_BLASTER_SHOTS; ++j) {
      blaster = &this->player.blaster[j];
      if (blaster->blaster_state == BLASTER_READY) {
        continue;
      }
      else if (blaster->depth == flipper->depth &&
               blaster->position == flipper->position) {
        // HIT!
        ut3k_play_sample(enemy_destroyed_soundkey);
        blaster->blaster_state = BLASTER_READY;
        flipper->flipper_state = DESTROYED;
        flippers_remaining--;
      }
    }

    // zapper check
    if (this->player.superzapper.superzapper_state == ZAPPER_FIRING) {
      if (flipper->depth == this->player.superzapper.depth &&
          this->player.superzapper.zapped_squares[flipper->position] == 1) {
        printf("flipper %d destroyed by superzapper\n", i);
        flipper->flipper_state = DESTROYED;
        flippers_remaining--;
      }
    }

    // any flipper active now has a chance to hit the player
    // flippers flipping between spaces don't hit the player
    if (flipper->flipper_state == ACTIVE &&
        flipper->depth == this->player.depth &&
        flipper->position == this->player.position) {
      // buh bye Player 1
      this->playfield.has_collision = 1;
      ut3k_play_sample(rand() & 0b1 ? crash_on_spike_soundkey : player_lose_life_soundkey);
      this->player_hit_and_restart.timer = default_flash_screen_timer;
      return GAME_PLAYER_HIT;
    }
  }

  if (this->player.superzapper.superzapper_state == ZAPPER_CHARGING &&
      this->player.superzapper.range > default_player_zapper_max_charge) {
    ut3k_play_sample(rand() & 0b1 ? crash_on_spike_soundkey : player_lose_life_soundkey);
    this->player_hit_and_restart.timer = default_flash_screen_timer;
    this->playfield.has_collision = 1;
    // TODO: play some electric explosion sound
    printf("overcharged ZAPPER!\n");
    return GAME_PLAYER_HIT;
  }

  return flippers_remaining == 0 ? GAME_LEVEL_UP : GAME_PLAY;
}



/** reset_player_and_nasties
 *
 * after stopping the game 'cause the player was hit or a level up,
 * prepare to resume.  Player can keep their position.  Nasties should
 * go back to inactive so they can respawn.
 * arg: full: set to 1 for a full reset, for level up.  Set to 0 for
 * recovering from a player being hit.
 */
static void reset_player_and_nasties(struct model *this, int full) {
  this->playfield.has_collision = 0;

  for (int i = 0; i < this->playfield.num_flippers; ++i) {
    if (full == 1 || this->playfield.flippers[i].flipper_state != DESTROYED) {
      this->playfield.flippers[i].flipper_state = INACTIVE;
    }
    this->playfield.flippers[i].position = 0;
    this->playfield.flippers[i].next_depth_timer = default_next_depth_timer;
    this->playfield.flippers[i].depth = this->playfield.num_arrays - 1;
  }

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    this->player.blaster[i].blaster_state = BLASTER_READY;
  }

  // TODO: investigate why player gets hit while firing
  if (full == 1 ||
      this->player.superzapper.superzapper_state == ZAPPER_CHARGING ||
      this->player.superzapper.superzapper_state == ZAPPER_FIRING) {
    this->player.superzapper.superzapper_state = ZAPPER_READY;
  }
}


/** spawn_flipper
 *
 * bring an INACTIVE flipper to life.
 * Set a random time for to go from spawning to active.
 */
static void spawn_flipper(struct model *this, int index) {
  struct flipper *flipper = &this->playfield.flippers[index];

  *flipper = (struct flipper const)
    {
     .flipper_state = SPAWNING,
     .position = 0,
     .next_depth_timer = default_next_depth_timer,
     .intra_depth_timer = 0,
     .movement_direction = rand() & 0b1 ? 1 : -1
    };
}



/** init_player_for_game
 *
 * start the player off ready for the game.
 */
static void init_player_for_game(struct player *player) {
  *player = (struct player const)
    {
     .position = 0,
     .depth = 0,
     .lives_remaining = 3,
     .superzapper= (struct superzapper const)
     {
      .range = 1,
      .zapped_range = 0,
      .timer = default_player_zapper_charging_timer
     }
    };

}

static void init_playfield_for_level(struct playfield *playfield, int level) {

  if (1 || level == 1) {
    *playfield = (struct playfield const)
      {
       .num_arrays = sizeof((((struct playfield *)0)->size_per_array)) / sizeof(int*),
       .size_per_array = { 28, 20, 12, 4, 4 },
       //.size_per_array = { 14, 10, 6, 4, 4 },
       .circular = 1,
       .num_flippers = 12,
       .next_flipper_spawn_timer = rand() % default_next_flipper_spawn_timer_randmoness + default_next_flipper_spawn_timer_minumum,
       .has_collision = 0
      };


    for (int i = 0; i < playfield->num_flippers; ++i) {
      // put flippers at max depth, inactive, and randomly positioned
      playfield->flippers[i].depth = playfield->num_arrays - 1;
      playfield->flippers[i].flipper_state = INACTIVE;
    }
  }
}


static const char *superzapper_recharged_message = "   SUPERZAPPER RECHARGE    ";
static void set_model_state_levelup(struct model *this) {
  this->game_state = GAME_LEVEL_UP;
  this->playfield.level_number++;
  this->levelup.scroll_timer = default_text_scroll_timer;
  this->levelup.animation_timer = default_display_slow_time;
  strcpy(this->levelup.messaging, superzapper_recharged_message);
  this->levelup.msg_ptr = this->levelup.messaging;
  this->levelup.init_levelup_display = 1;
  this->levelup.animation_state = DIAG_RISING_SLOW;
  this->levelup.animation_iterations = 0;
  this->levelup.animation_iterations_max = &default_display_iterations_slow_med;

  reset_player_and_nasties(this, 1);
}


static inline int index_on_depth(int from, int sizeof_from, int sizeof_to) {
  return from * sizeof_to / sizeof_from;
}


