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

#ifndef VIEW_COMMON_H
#define VIEW_COMMON_H

#include "ut3k_view.h"
#include "model.h"


#define PLAYER_BLINK(clock)  (clock % player_blink_cycle) < (player_blink_cycle_on)
#define MOTO_BLINK(clock)  (clock % moto_blink_cycle) < (moto_blink_cycle_on)

void draw_stores_as_leds(const struct stores *stores, struct display *leds, uint32_t clock);

extern const int player_blink_cycle;
extern const int player_blink_cycle_on;
extern const int moto_blink_cycle;
extern const int moto_blink_cycle_on;


#endif
