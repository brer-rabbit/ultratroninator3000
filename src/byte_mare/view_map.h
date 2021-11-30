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

#ifndef VIEW_MAP_H
#define VIEW_MAP_H

struct view_map;

#include "ut3k_view.h"
#include "model.h"
#include "controller_map.h"


struct view_map* create_view_map(struct ut3k_view *ut3k_view);
void free_view_map(struct view_map*);

void clear_view_map(struct view_map *this);

void toggle_map_display(struct view_map *this);
void draw_player(struct view_map *this, const struct xy *quadrant, uint32_t clock);
void draw_moto_groups(struct view_map *this, const struct moto_group *moto_groups);

void render_map_display(struct view_map*, uint32_t clock);

#endif
