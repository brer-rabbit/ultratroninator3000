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


/* Listeners / callback */

typedef void (*f_view_control_panel_listener)(const struct control_panel *control_panel, void *userdata);

/** create_alphanum_ut3k_view
 *
 * "constructor"
 * start the view off with a particular view opens HT16K33.
 * This does a read from the HT16K33: the caller must wait >~20 ms
 * before any other reads against this chip.  Such as update_view().
 *
 */
struct ut3k_view* create_alphanum_ut3k_view();


/** free resources from game view
 * well technically, all games are set to free play here...
 */
int free_ut3k_view(struct ut3k_view *this);


/** 
 * Keyscan all controls.
 * Keyscan results are available via the callback registered listener
 * or via the get_control_panel call.
 * This function must only be called not more than every ~20ms,
 * otherwise the caller risks getting invalid results from the chip.
 */
void update_controls(struct ut3k_view*, uint32_t clock);


 /**
  * Register event handlers with the appropriate components.
  * The control_panel_listener will be called during update_view.
  */
void register_control_panel_listener(struct ut3k_view *view, f_view_control_panel_listener f, void *userdata);


/** get the control panel.  Useful for initialization or if you really
 *   want to avoid the callback model.
 */
const struct control_panel* get_control_panel(struct ut3k_view*);



/** show the displays
 * update all 3 displays, the rows of LEDs.
 * This function may be called every ~10ms.
 */
void update_displays(struct ut3k_view*, struct display_strategy*, uint32_t clock);


#endif
