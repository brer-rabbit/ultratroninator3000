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



void draw_flight(struct view_flight *this, void *scroller, f_animator animation) {

}


void render_flight_display(struct view_flight *this, uint32_t clock) {
  clear_ut3k_display(&this->ut3k_display);
}

