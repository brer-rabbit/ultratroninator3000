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
 * get the backpacks up and going. Clear their mem first.
 **/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "pong.h"
#include "model.h"
#include "view.h"
#include "ut3k_pulseaudio.h"

static const int field_x_max = 19;
static const int field_x_min = 0;
static const int field_y_max = 5;
static const int field_y_min = 0;
static const int default_x_move_timer = 12;
static const int default_y_move_timer = 77;
static const int default_win_score = 2;


static void init_ball(struct model *this);
static void clocktick_ball(struct model *this);
static void collision_detect(struct model*);


struct model {
  game_state_t game_state;
  struct player player1;
  struct player player2;
  struct ball ball;
  struct player *server; // who is serving the ball
};




/** create the model.
 */
struct model* create_model() {
  struct model* this = (struct model*)malloc(sizeof(struct model));
  *this = (struct model const) { 0 };

  // reasonable defaults for players & ball
  this->player1.y_position = (field_y_max - field_y_min) / 2;
  this->player2.y_position = (field_y_max - field_y_min) / 2;

  // player 1 serves
  this->server = &this->player1;
  init_ball(this);
  this->game_state = GAME_ATTRACT;

  return this;
}


void free_model(struct model *this) {
  return free(this);
}


game_state_t clocktick_model(struct model *this) {
  // update the ball
  clocktick_ball(this);
  collision_detect(this);

  if (this->game_state == GAME_SERVE &&
      (this->player1.score == default_win_score ||
       this->player2.score == default_win_score)) {
    this->game_state = GAME_OVER;
  }
  return this->game_state;
}

game_state_t get_game_state(struct model *this) {
  return this->game_state;
}


void player1_move(struct model *this, int distance) {
  if (this->game_state == GAME_SERVE ||
      this->game_state == GAME_PLAY) {
    if (distance > 0) {
      this->player1.y_position =
        this->player1.y_position + distance > field_y_max ?
        field_y_max : this->player1.y_position + distance;
    }
    else if (distance < 0) {
      this->player1.y_position =
        this->player1.y_position + distance < field_y_min ?
        field_y_min : this->player1.y_position + distance;
    }
  }
}



void player2_move(struct model *this, int distance) {
  if (this->game_state == GAME_SERVE ||
      this->game_state == GAME_PLAY) {
    if (distance > 0) {
      this->player2.y_position =
        this->player2.y_position + distance > field_y_max ?
        field_y_max : this->player2.y_position + distance;
    }
    else if (distance < 0) {
      this->player2.y_position =
        this->player2.y_position + distance < field_y_min ?
        field_y_min : this->player2.y_position + distance;
    }
  }
}


void player1_button_pushed(struct model *this) {
  if (this->game_state == GAME_SERVE &&
      this->server == &this->player1) {
    this->game_state = GAME_PLAY;
    ut3k_play_sample(serve1_soundkey);
  }
  else if (this->game_state == GAME_OVER) {
    this->game_state = GAME_ATTRACT;
  }
  else if (this->game_state == GAME_ATTRACT) {
    // reset for next game
    this->player1.score = 0;
    this->player2.score = 0;
    this->game_state = GAME_SERVE;
  }
}

void player2_button_pushed(struct model *this) {
  if (this->game_state == GAME_SERVE &&
      this->server == &this->player2) {
    this->game_state = GAME_PLAY;
    ut3k_play_sample(serve1_soundkey);
  }
  else if (this->game_state == GAME_OVER) {
    this->game_state = GAME_ATTRACT;
    printf("set state from over to attract\n");
  }
  else if (this->game_state == GAME_ATTRACT) {
    this->player1.score = 0;
    this->player2.score = 0;
    this->game_state = GAME_SERVE;
    printf("set state from attract to serve\n");
  }
}


const struct player* get_player1(struct model *this) {
  return (const struct player*) &this->player1;
}
const struct player* get_player2(struct model *this) {
  return (const struct player*) &this->player2;
}

const struct ball* get_ball(struct model *this) {
  return (const struct ball*) &this->ball;
}


/* static ----------------------------------------------------------- */


static void clocktick_ball(struct model *this) {
  struct ball *ball = &this->ball;

  if (this->game_state == GAME_PLAY) {
    if (--ball->x_move_timer == 0) {
      // move the ball in the X direction
      ball->x_move_timer = ball->x_clocks_per_move;
      ball->x_position += ball->x_direction;
    }
  
    if (--ball->y_move_timer == 0) {
      // move the ball in the Y direction
      ball->y_move_timer = ball->y_clocks_per_move;
      ball->y_position += ball->y_direction;
    }
  }
  else if (this->game_state == GAME_SERVE) {
    ball->y_position = this->server->y_position;
  }
}


static void collision_detect(struct model *this) {
  struct ball *ball = &this->ball;
  struct player *player1 = &this->player1;
  struct player *player2 = &this->player2;

  if (this->game_state != GAME_PLAY) {
    return;
  }

  // check if the ball has hit a player paddle
  // criteria is ball position and direction
  if (ball->x_position == field_x_min &&
      ball->x_direction == -1) {
    // it's either be volleyed back by player1 or going out of bounds
    if (player1->y_position == ball->y_position) {
      ball->x_direction = 1;
      ball->x_clocks_per_move--;
      printf("player1 hit the ball: clock/move: %d\n", ball->x_clocks_per_move);
      // TODO MAGIC NUMBER
      ut3k_play_sample( hit_soundkeys[ rand() % 4 ] );
    }
    else {
      printf("player1 missed the ball!\n");
      this->game_state = GAME_SERVE;
      this->server = &this->player2;
      this->player2.score++;
      init_ball(this);
      // TODO MAGIC NUMBER
      ut3k_play_sample( miss_soundkeys[rand() % 3] );
    }
  }
    
  if (ball->x_position == field_x_max &&
      ball->x_direction == 1) {
    // it's either be volleyed back by player1 or going out of bounds
    if (player2->y_position == ball->y_position) {
      ball->x_direction = -1;
      ball->x_clocks_per_move--;
      printf("player2 hit the ball: clock/move: %d\n", ball->x_clocks_per_move);
      ut3k_play_sample( hit_soundkeys[ rand() % 4 ] );
    }
    else {
      printf("player2 missed the ball!\n");
      this->game_state = GAME_SERVE;
      this->server = &this->player1;
      this->player1.score++;
      init_ball(this);
      ut3k_play_sample( miss_soundkeys[ rand() % 4 ] );
    }
  }


  // bounce off the top and bottom
  if (ball->y_position == field_y_min &&
      ball->y_direction == -1) {
    ball->y_direction = 1;
  }
  else if (ball->y_position == field_y_max &&
      ball->y_direction == 1) {
    ball->y_direction = -1;
  }

}


void init_ball(struct model *this) {
  struct ball *ball = &this->ball;

  // ball start next to whoever is serving and moves to the opposite player
  if (this->server == &this->player1) {
    ball->x_position = field_x_min + 1;
    ball->x_direction = 1;
  }
  else {
    ball->x_position = field_x_max - 1;
    ball->x_direction = -1;
  }

  ball->y_position = this->server->y_position;
  ball->y_direction = rand() % 2 == 0 ? -1 : 1;

  ball->x_clocks_per_move = default_x_move_timer;
  ball->x_move_timer = ball->x_clocks_per_move;
  ball->y_clocks_per_move = default_y_move_timer;
  ball->y_move_timer = ball->y_clocks_per_move;
}
