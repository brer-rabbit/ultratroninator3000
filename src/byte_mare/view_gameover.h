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

#ifndef VIEW_GAMEOVER_H
#define VIEW_GAMEOVER_H

struct view_gameover;

#include "ut3k_view.h"
#include "controller_gameover.h"


struct view_gameover* create_view_gameover(struct ut3k_view *ut3k_view);
void free_view_gameover(struct view_gameover*);

void clear_view_gameover(struct view_gameover *this);

void toggle_gameover_display(struct view_gameover *this);
void draw_gameover(struct view_gameover*, void*, f_animator);

void render_gameover_display(struct view_gameover*, uint32_t clock);

#endif
