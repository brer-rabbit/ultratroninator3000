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
#include "view_flight.h"
#include "view_common.h"


static const int display_width = 20;
static const uint16_t tunnel_lower[] = { SEG_E, SEG_L, SEG_M, SEG_N, SEG_C };
static const uint16_t tunnel_upper[] = { SEG_F, SEG_H, SEG_J, SEG_K, SEG_B };
static const int display_map[] = { 2, 1, 0 };


struct view_flight {
  struct ut3k_view *ut3k_view;
  struct ut3k_display ut3k_display;
};


struct view_flight* create_view_flight(struct ut3k_view *ut3k_view) {
  struct view_flight *this = (struct view_flight*)malloc(sizeof(struct view_flight));
  this->ut3k_view = ut3k_view;
  reset_ut3k_display(&this->ut3k_display);

  return this;
}


void free_view_flight(struct view_flight *this) {
  free(this);
}



void clear_view_flight(struct view_flight *this) {
  reset_ut3k_display(&this->ut3k_display);
}


void set_flight_messaging(struct view_flight *this, void *scroller, f_animator animation) {
  this->ut3k_display.displays[0].display_type = string_display;
  this->ut3k_display.displays[0].f_animate = animation;
  this->ut3k_display.displays[0].userdata = scroller;
}


void draw_flight_tunnel(struct view_flight *this, const struct flight_path *flight_path, const struct player *player, uint32_t clock) {
  int y = player->flight_tunnel.y;
  int x = player->flight_tunnel.x;
  const struct flight_path_slice *lower, *upper;

  // iterate over each display
  for (int i = 0; i < 3; ++i) {
    if (y + i*2 >= MAX_FLIGHT_PATH_LENGTH) {
      break;
    }

    // fill in digits lower half up to offset
    lower = &flight_path->slice[y + i*2];
    upper = &flight_path->slice[y + i*2 + 1];

    for (int j = 0; j < lower->offset; ++j) {
      this->ut3k_display.displays[ display_map[i] ].display_value.display_glyph[j / 5] |=
        tunnel_lower[ j % 5 ];
    }
    // fill in digits lower half to remaining width
    for (int j = lower->offset + lower->width; j < display_width; ++j) {
      this->ut3k_display.displays[ display_map[i] ].display_value.display_glyph[j / 5] |=
        tunnel_lower[ j % 5 ];
    }

    // fill in digits upper half up to offset
    for (int j = 0; j < upper->offset; ++j) {
      this->ut3k_display.displays[ display_map[i] ].display_value.display_glyph[j / 5] |=
        tunnel_upper[ j % 5 ];
    }
    // fill in digits upper half to remaining width
    for (int j = upper->offset + upper->width; j < display_width; ++j) {
      this->ut3k_display.displays[ display_map[i] ].display_value.display_glyph[j / 5] |=
        tunnel_upper[ j % 5 ];
    }

  }

  // draw the player at a different intensity so player is
  // differentiated versus walls
  if (clock % player_blink_cycle < player_blink_cycle_on) {
    this->ut3k_display.displays[2].display_value.display_glyph[x / 5] |=
      tunnel_lower[ x % 5 ];
  }
  

  draw_stores_as_leds(&player->stores, &this->ut3k_display.leds, clock);
}


void render_flight_display(struct view_flight *this, uint32_t clock) {
  commit_ut3k_view(this->ut3k_view, &this->ut3k_display, clock);
  clear_ut3k_display(&this->ut3k_display);
}

