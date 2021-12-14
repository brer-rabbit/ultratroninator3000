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

#include "view_common.h"



const int player_blink_cycle = 8;
const int player_blink_cycle_on = 4;
const int moto_blink_cycle = 32;
const int moto_blink_cycle_on = 20;

void draw_stores_as_leds(const struct stores *stores, struct display *leds, uint32_t clock) {
  
  set_red_leds(leds, (1 << (stores->bullets / 8)) - 1);
  set_blue_leds(leds, (1 << (stores->fuel / 8)) - 1);
  set_green_leds(leds, (1 << ((stores->shields / 2) + (stores->shields % 2 && (clock & 0b1000)))) - 1);
}
