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


/* calc_controller.h
 *
 */

#ifndef CALC_CONTROLLER_H
#define CALC_CONTROLLER_H

struct calc_controller;

#include "calc_model.h"
#include "ut3k_view.h"
#include "control_panel.h"


struct calc_controller* create_calc_controller(struct calc_model*, struct ut3k_view*);
void free_calc_controller(struct calc_controller *this);


void controller_update(struct calc_controller *this, uint32_t clock);

void controller_initialize_control_panel(struct control_panel *control_panel, void *userdata);
void controller_callback_control_panel(const struct control_panel *control_panel, void *userdata);

#endif
