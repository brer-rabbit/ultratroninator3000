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
#include "view_battle.h"
#include "view_common.h"

static void draw_landscape(struct view_battle *this, const struct battle *battle);
static void draw_motogroup(struct view_battle *this, const struct battle *battle, uint32_t clock);
static void draw_single_segment_object(struct view_battle *this, const struct sector *sector);


// display of the ground.  With a digit broken into two square,
// segments vary depending on if it's on the lower or upper part of
// the digit
static const uint16_t ground0_lower[] =
  {
   SEG_C,
   SEG_C | SEG_N,
   SEG_C | SEG_N | SEG_M,
   SEG_C | SEG_N | SEG_M | SEG_L,
   SEG_C | SEG_N | SEG_M | SEG_L | SEG_E,
   SEG_C | SEG_N | SEG_M | SEG_L | SEG_E,
   SEG_C | SEG_N | SEG_M | SEG_L | SEG_E
  };

static const uint16_t ground1_lower[] =
  {
   0, 0, 0, 0, 0,
   SEG_C,
   SEG_C | SEG_N,
   SEG_C | SEG_N | SEG_M
  };

static const uint16_t ground0_upper[] =
  {
   SEG_B,
   SEG_B | SEG_K,
   SEG_B | SEG_K | SEG_J,
   SEG_B | SEG_K | SEG_J | SEG_H,
   SEG_B | SEG_K | SEG_J | SEG_H | SEG_F,
   SEG_B | SEG_K | SEG_J | SEG_H | SEG_F,
   SEG_B | SEG_K | SEG_J | SEG_H | SEG_F
  };

static const uint16_t ground1_upper[] =
  {
   0, 0, 0, 0, 0,
   SEG_B,
   SEG_B | SEG_K,
   SEG_B | SEG_K | SEG_J
  };

static const int display_map[] = { 2, 1, 0 };


static const uint16_t object_xz_lower_glyph[] =
  { SEG_C, SEG_N, SEG_M, SEG_L, SEG_E };
static const uint16_t object_xz_upper_glyph[] =
  { SEG_B, SEG_K, SEG_J, SEG_H, SEG_F };



struct view_battle {
  struct ut3k_view *ut3k_view;
  struct ut3k_display ut3k_display;
};


struct view_battle* create_view_battle(struct ut3k_view *ut3k_view) {
  struct view_battle *this = (struct view_battle*)malloc(sizeof(struct view_battle));
  this->ut3k_view = ut3k_view;
  reset_ut3k_display(&this->ut3k_display);

  return this;
}


void free_view_battle(struct view_battle *this) {
  free(this);
}



void clear_view_battle(struct view_battle *this) {
  reset_ut3k_display(&this->ut3k_display);
}



void draw_battle(struct view_battle *this, const struct battle *battle, uint32_t clock) {
  draw_landscape(this, battle);
  draw_motogroup(this, battle, clock);
}


void draw_battle_player(struct view_battle *this, const struct player *player, uint32_t clock) {
  if (clock % player_blink_cycle < player_blink_cycle_on) {
    draw_single_segment_object(this, &player->sector);
  }

  draw_stores_as_leds(&player->stores, &this->ut3k_display.leds, clock);
}

void render_battle_display(struct view_battle *this, uint32_t clock) {
  commit_ut3k_view(this->ut3k_view, &this->ut3k_display, clock);
  clear_ut3k_display(&this->ut3k_display);
}




static void draw_landscape(struct view_battle *this, const struct battle *battle) {

  for (int i = 0; i < 3; i++) {
    this->ut3k_display.displays[ display_map[i] ].display_value.display_glyph[1] |=
      ground0_lower[ battle->terrain_map[i * 2] ];
    this->ut3k_display.displays[ display_map[i] ].display_value.display_glyph[0] |=
      ground1_lower[ battle->terrain_map[i * 2] ];

    this->ut3k_display.displays[ display_map[i] ].display_value.display_glyph[1] |=
      ground0_upper[ battle->terrain_map[i * 2 + 1] ];
    this->ut3k_display.displays[ display_map[i] ].display_value.display_glyph[0] |=
      ground1_upper[ battle->terrain_map[i * 2 + 1] ];
  }

}


static void draw_motogroup(struct view_battle *this, const struct battle *battle, uint32_t clock) {
}

static void draw_single_segment_object(struct view_battle *this, const struct sector *sector) {
  int display;
  int digit;
  int is_lower;
  int glyph;

  // left half of display is y-z values, y goes vertical and
  // z is left right to show elevation.  This goes on digits 2 or 3 depending
  // on y-location.
  display = display_map[ player->sector.y / 2 ];
  is_lower = player->sector.y % 2 == 0 ? 1 : 0;
  digit = player->sector.z > 4 ? 0 : 1;
  glyph = is_lower ?
    object_xz_lower_glyph[ player->sector.z % 5 ] :
    object_xz_upper_glyph[ player->sector.z % 5 ];

  this->ut3k_display.displays[ display ].display_value.display_glyph[ digit ] |=glyph;

  // right half of the display is x-y values, y again going vertical.
  // this gives a top-down view
  digit = player->sector.x > 4 ? 2 : 3;
  glyph = is_lower ?
    object_xz_lower_glyph[ player->sector.x % 5 ] :
    object_xz_upper_glyph[ player->sector.x % 5 ];

  this->ut3k_display.displays[ display ].display_value.display_glyph[ digit ] |=glyph;

}
