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


/* controller.h
 *
 */

#ifndef CONTROLLER_GAMEOVER_H
#define CONTROLLER_GAMEOVER_H

struct controller_gameover;
struct clock_scroller;
struct manual_scroller;

#include "model.h"
#include "ut3k_view.h"
#include "view_gameover.h"
#include "control_panel.h"


struct controller_gameover* create_controller_gameover(struct model*, struct view_gameover*, struct ut3k_view*);
void free_controller_gameover(struct controller_gameover *this);


void controller_gameover_update(struct controller_gameover *this, uint32_t clock);

void controller_gameover_callback_control_panel(const struct control_panel *control_panel, void *userdata);


#endif
