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
#include <string.h>
#include "view.h"


// f_animator for LEDs
static void f_attract_leds_animator(struct display *display, uint32_t clock);

struct view {
  struct ut3k_view *ut3k_view;
  struct ut3k_display ut3k_display;
};


struct view* create_pong_view(struct ut3k_view *ut3k_view) {
  struct view *this = (struct view*)malloc(sizeof(struct view));
  this->ut3k_view = ut3k_view;
  reset_ut3k_display(&this->ut3k_display);

  return this;
}


void free_pong_view(struct view *this) {
  free(this);
}


void clear_view(struct view *this) {
  reset_ut3k_display(&this->ut3k_display);
}



void draw_gameover(struct view *this, void *scroller, f_animator animation, int p1_score, int p2_score) {
  this->ut3k_display.displays[0].f_animate = animation;
  this->ut3k_display.displays[0].userdata = scroller;

  this->ut3k_display.displays[1].display_type = integer_display;
  this->ut3k_display.displays[1].display_value.display_int = p2_score;

  this->ut3k_display.displays[2].display_type = integer_display;
  this->ut3k_display.displays[2].display_value.display_int = p1_score;

  this->ut3k_display.displays[ p1_score > p2_score ? 2 : 1].blink = HT16K33_BLINK_FAST;
}


void draw_attract(struct view *this, void *scroller, f_animator animation) {
  // rotate through the displays-- figure out which
  // one has animation tand set it to the next one.
  // if none do (initial state), set it to the first.
  if (this->ut3k_display.displays[0].f_animate == animation) {
    this->ut3k_display.displays[0].f_animate = NULL;
    this->ut3k_display.displays[1].f_animate = animation;
    this->ut3k_display.displays[1].userdata = scroller;
  }
  else if (this->ut3k_display.displays[1].f_animate == animation) {
    this->ut3k_display.displays[1].f_animate = NULL;
    this->ut3k_display.displays[2].f_animate = animation;
    this->ut3k_display.displays[2].userdata = scroller;
  }
  else if  (this->ut3k_display.displays[2].f_animate == animation) {
    this->ut3k_display.displays[2].f_animate = NULL;
    this->ut3k_display.displays[0].f_animate = animation;
    this->ut3k_display.displays[0].userdata = scroller;
  }
  else {
    // initial case
    this->ut3k_display.displays[0].f_animate = animation;
    this->ut3k_display.displays[0].userdata = scroller;
    this->ut3k_display.displays[0].blink = HT16K33_BLINK_OFF;
    this->ut3k_display.displays[1].f_animate = NULL;
    this->ut3k_display.displays[1].blink = HT16K33_BLINK_OFF;
    this->ut3k_display.displays[2].f_animate = NULL;
    this->ut3k_display.displays[2].blink = HT16K33_BLINK_OFF;
  }
  
}


void draw_player1_paddle(struct view *this, int y_position, int handicap) {
  int display;
  int glyph;

  for (int i = y_position; i <= y_position + handicap; ++i) {
    display = i / 2;
    glyph = i % 2;

    this->ut3k_display.displays[display].display_type = glyph_display;
    this->ut3k_display.displays[display].display_value.display_glyph[0] |=
      glyph ? SEG_E : SEG_F;
  }
}


void draw_player2_paddle(struct view *this, int y_position, int handicap) {
  int display;
  int glyph;

  for (int i = y_position; i <= y_position + handicap; ++i) {
    display = i / 2;
    glyph = i % 2;
    this->ut3k_display.displays[display].display_type = glyph_display;
    this->ut3k_display.displays[display].display_value.display_glyph[3] |=
      glyph ? SEG_C : SEG_B;
  }
}


static const uint16_t ball_glyph[2][5] =
  {
   { SEG_F, SEG_H, SEG_J, SEG_K, SEG_B },
   { SEG_E, SEG_L, SEG_M, SEG_N, SEG_C }
  };

void draw_ball(struct view *this, int x, int y) {
  int digit = x / 5;
  int horiz_position = x % 5;
  int display = y / 2;
  int vert_position = y % 2;

  this->ut3k_display.displays[display].display_type = glyph_display;
  this->ut3k_display.displays[display].display_value.display_glyph[digit] |=
    ball_glyph[vert_position][horiz_position];
}


void draw_player1_score(struct view *this, int score) {
  // light a number of LEDs to match the score
  set_red_leds(&this->ut3k_display.leds, (1 << score) - 1);
}
void draw_player2_score(struct view *this, int score) {
  set_blue_leds(&this->ut3k_display.leds, (1 << score) - 1);
}



void render_view(struct view *this, uint32_t clock) {
  commit_ut3k_view(this->ut3k_view, &this->ut3k_display, clock);
  // wipe after render
  clear_ut3k_display(&this->ut3k_display);
}

