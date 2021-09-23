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


/* ut3k_view.h
 *
 */

#ifndef UT3K_VIEW_H
#define UT3K_VIEW_H

struct ut3k_view;

#include "display_strategy.h"
#include "ht16k33.h"
#include "control_panel.h"

/** create_alphanum_ut3k_view
 *
 * "constructor"
 * start the view off with a particular view opens HT16K33.
 * 
 */
struct ut3k_view* create_alphanum_ut3k_view();


/** free resources from game view
 * well technically, all games are set to free play here...
 */
int free_ut3k_view(struct ut3k_view *this);


/** show the view of the model-
 * update all 3 displays and any other visible components
 * Return current state of control panel.
 */
void update_view(struct ut3k_view*, struct display_strategy*);


/* Listeners / callback */

/** callback for rotary encoder if something happened.
 * by something- either rotation or button push.  Callback provide:
 * delta (-1, 0, 1) for direction of change
 * 0/1 button state is not pushed / pushed
 * 0/1 if the button changed state
 * user data in a void* pointer
 */
typedef void (*f_controller_update_rotary_encoder)(int8_t delta, uint8_t button_pushed, uint8_t button_changed, void*);
typedef void (*f_view_control_panel_listener)(struct control_panel *control_panel, void *userdata);



 /**
  * Register event handlers with the appropriate components
  */
void register_green_encoder_listener(struct ut3k_view *view, f_controller_update_rotary_encoder f, void *userdata);
 
void register_control_panel_listener(struct ut3k_view *view, f_view_control_panel_listener f, void *userdata);


#endif
