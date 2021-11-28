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
#include "view_gameover.h"


struct view_gameover {
  struct ut3k_view *ut3k_view;
  struct ut3k_display ut3k_display;
};


struct view_gameover* create_view_gameover(struct ut3k_view *ut3k_view) {
  struct view_gameover *this = (struct view_gameover*)malloc(sizeof(struct view_gameover));
  this->ut3k_view = ut3k_view;
  reset_ut3k_display(&this->ut3k_display);

  return this;
}


void free_view_gameover(struct view_gameover *this) {
  free(this);
}



void clear_view_gameover(struct view_gameover *this) {
  reset_ut3k_display(&this->ut3k_display);
}



void draw_gameover(struct view_gameover *this, void *scroller, f_animator animation) {

}


void render_gameover_display(struct view_gameover *this, uint32_t clock) {
  clear_ut3k_display(&this->ut3k_display);
}

