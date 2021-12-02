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
#include "view_map.h"


struct view_map {
  struct ut3k_view *ut3k_view;
  struct ut3k_display ut3k_display;
};


static const uint16_t player_top_glyph = SEG_F | SEG_A | SEG_B;
static const uint16_t player_bottom_glyph = SEG_E | SEG_D | SEG_C;
static const uint16_t moto_top_glyph = SEG_H;
static const uint16_t moto_bottom_glyph = SEG_L;

static const int cursor_flash_cycle = 8;
static const int cursor_flash_duty_cycle = cursor_flash_cycle / 2;

// coordinates are mapped from bottom to top as 0,1,2 while displays are
// numerically 2,1,0 from bottom to top.  Create a mapping to index.
static const int display_map[] = { 2, 1, 0 };




struct view_map* create_view_map(struct ut3k_view *ut3k_view) {
  struct view_map *this = (struct view_map*)malloc(sizeof(struct view_map));
  this->ut3k_view = ut3k_view;
  reset_ut3k_display(&this->ut3k_display);

  return this;
}


void free_view_map(struct view_map *this) {
  free(this);
}



void clear_view_map(struct view_map *this) {
  reset_ut3k_display(&this->ut3k_display);
}



/** draw_player
 *
 * place the player on the map by the given xy coordinate
 */

void draw_player(struct view_map *this, const struct player *player, uint32_t clock) {
  int display, is_top;
  int digit;

  display = display_map[ player->quadrant.y / 2 ];
  is_top = player->quadrant.y & 0b1; // top or bottom part of digit?
  digit = player->quadrant.x;

  this->ut3k_display.displays[display].display_type = glyph_display;
  this->ut3k_display.displays[display].display_value.display_glyph[digit] |=
    is_top ? player_top_glyph : player_bottom_glyph;

  if (clock % cursor_flash_cycle < cursor_flash_duty_cycle) {
    display = display_map[ player->cursor_quadrant.y / 2 ];
    is_top = player->cursor_quadrant.y & 0b1; // top or bottom part of digit?
    digit = player->cursor_quadrant.x;

    this->ut3k_display.displays[display].display_type = glyph_display;
    this->ut3k_display.displays[display].display_value.display_glyph[digit] |=
      is_top ? player_top_glyph : player_bottom_glyph;
  }
}


void draw_moto_groups(struct view_map *this, const struct moto_group *moto_groups) {
  int display, is_top;
  int digit;

  for (int i = 0; i < MAX_MOTO_GROUPS; ++i) {
    if (moto_groups[i].status == ACTIVE) {
      display = display_map[ moto_groups[i].quadrant.y / 2 ];
      is_top = moto_groups[i].quadrant.y & 0b1;
      digit = moto_groups[i].quadrant.x;

      this->ut3k_display.displays[display].display_type = glyph_display;
      this->ut3k_display.displays[display].display_value.display_glyph[digit] |=
        is_top ? moto_top_glyph : moto_bottom_glyph;
    }
  }
}


void render_map_display(struct view_map *this, uint32_t clock) {
  commit_ut3k_view(this->ut3k_view, &this->ut3k_display, clock);
  clear_ut3k_display(&this->ut3k_display);
}

