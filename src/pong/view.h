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

#ifndef VIEW_H
#define VIEW_H

struct view;

#include "ut3k_view.h"
#include "controller.h"


struct view* create_pong_view(struct ut3k_view *ut3k_view);
void free_pong_view(struct view*);
void clear_view(struct view *this);

void draw_gameover(struct view *this, void *scroller, f_animator animation, int p1_score, int p2_score);
void draw_attract(struct view *this, void *scroller, f_animator animation);

void draw_player1_paddle(struct view *this, int y_position, int handicap);
void draw_player2_paddle(struct view *this, int y_position, int handicap);
void draw_ball(struct view *this, int x, int y);
void draw_player1_score(struct view *this, int score);
void draw_player2_score(struct view *this, int score);


void render_view(struct view*, uint32_t clock);

#endif
