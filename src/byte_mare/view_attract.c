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
#include "view_attract.h"


struct view_attract {
  struct ut3k_view *ut3k_view;
  struct ut3k_display ut3k_display;
  int attract_display_target;
};


struct view_attract* create_view_attract(struct ut3k_view *ut3k_view) {
  struct view_attract *this = (struct view_attract*)malloc(sizeof(struct view_attract));
  this->ut3k_view = ut3k_view;
  this->attract_display_target = 0;
  reset_ut3k_display(&this->ut3k_display);

  return this;
}


void free_view_attract(struct view_attract *this) {
  free(this);
}



void clear_view_attract(struct view_attract *this) {
  reset_ut3k_display(&this->ut3k_display);
}


// change the alphanum display from 0 to 2 (or vice versa) for the attract text
void toggle_attract_display(struct view_attract *this) {
  this->ut3k_display.displays[this->attract_display_target].f_animate = NULL;
  this->ut3k_display.displays[this->attract_display_target].userdata = NULL;
  this->attract_display_target = this->attract_display_target == 0 ? 2 : 0;
}

void draw_attract(struct view_attract *this, void *scroller, f_animator animation) {
  this->ut3k_display.displays[this->attract_display_target].f_animate = animation;
  this->ut3k_display.displays[this->attract_display_target].userdata = scroller;
}


void render_attract_display(struct view_attract *this, uint32_t clock) {
  commit_ut3k_view(this->ut3k_view, &this->ut3k_display, clock);
  clear_ut3k_display(&this->ut3k_display);
}



