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

#ifndef VIEW_FLIGHT_H
#define VIEW_FLIGHT_H

struct view_flight;

#include "ut3k_view.h"
#include "controller_flight.h"


struct view_flight* create_view_flight(struct ut3k_view *ut3k_view);
void free_view_flight(struct view_flight*);

void clear_view_flight(struct view_flight *this);


void set_flight_messaging(struct view_flight *this, void *scroller, f_animator animation);
void draw_flight_tunnel(struct view_flight *this, const struct flight_path *flight_path, const struct player *player, uint32_t clock);

void render_flight_display(struct view_flight*, uint32_t clock);

#endif
