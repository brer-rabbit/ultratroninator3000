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

#ifndef VIEW_ATTRACT_H
#define VIEW_ATTRACT_H

struct view_attract;

#include "ut3k_view.h"
#include "controller_attract.h"


struct view_attract* create_view_attract(struct ut3k_view *ut3k_view);
void free_view_attract(struct view_attract*);

void clear_view_attract(struct view_attract *this);

void toggle_attract_display(struct view_attract *this);
void draw_attract(struct view_attract*, void*, f_animator);

void render_attract_display(struct view_attract*, uint32_t clock);

#endif
