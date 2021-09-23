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


/* control_panel.h
 *
 */

#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include <stdint.h>

#include "ht16k33.h"

struct control_panel;

/** create_control_panel
 *
 * construct a control panel.  This object is reponsible for state of
 * all user interface gadgets on the front panel.
 */
struct control_panel* create_control_panel();


/** free any resources allocated for the control panel
 */
void free_control_panel(struct control_panel *this);


/** update_control_panel
 *
 * take the 6 bytes from the HT16K33 and map it to something
 * interpretable in terms of buttons and cool stuff.
 * this function is expected to be called at regular intervals:
 * the counts that are provided in the various accessor methods
 * are based upon calls to update_panel to increment the counts.
 */
int update_control_panel(struct control_panel *this, ht16k33keyscan_t keyscan);


/** get_green_selector
 * get the change in the selector value and it's button state
 */
void get_green_selector(struct control_panel *this, int *encoder_delta, int *button_state);


#endif // CONTROL_PANEL_H
