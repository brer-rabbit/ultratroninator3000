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
#include "view_map.h"


struct view_map {
  struct ut3k_view *ut3k_view;
  struct ut3k_display ut3k_display;
};


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



void draw_map(struct view_map *this, void *scroller, f_animator animation) {

}


void render_map_display(struct view_map *this, uint32_t clock) {
  clear_ut3k_display(&this->ut3k_display);
}

