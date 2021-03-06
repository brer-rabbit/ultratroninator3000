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

#ifndef VIEW_BATTLE_H
#define VIEW_BATTLE_H

struct view_battle;

#include "ut3k_view.h"
#include "controller_battle.h"


struct view_battle* create_view_battle(struct ut3k_view *ut3k_view);
void free_view_battle(struct view_battle*);

void clear_view_battle(struct view_battle *this);

void toggle_battle_display(struct view_battle *this);
void draw_battle(struct view_battle*, const struct battle*, uint32_t);
void draw_battle_player(struct view_battle*, const struct player*, uint32_t);

void render_battle_display(struct view_battle*, uint32_t clock);

#endif
