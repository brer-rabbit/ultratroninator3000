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


/* display interface for decoupling view & model --
 * this is somewhere between a strategy and a (broken?) bridge...
 * Allow for decoupling of the model from the view, and
 * at the same time provide the model with flexiblity to
 * vary the display based upon model data.
 *
 * Consider this a poor man's interface/abstract base class...
 */
#ifndef DISPLAY_STRATEGY_H
#define DISPLAY_STRATEGY_H


#include <stdint.h>
#include "ht16k33.h"
#include "ut3k_defs.h"
#include "ut3k_view.h"


struct display_strategy;

/** methods to implement to realize the strategy for getting a
 * particular display:
 * User of this interface will implement three
 * f_get_display types, one for each display.
 * (not) sorry for pulling in ht16k33.h types...a bit more exposure than I'd liek.
 * params the implementor of this will take:
 * struct display_strategy* : "this" struct display_strategy
 * display_value* : value to display on the ht16k33, set as one type of a union
 * ht16k33blink_t* : blink enum from ht16k33.h
 * ht16k33brightness_t* : brightness enum from ht16k33.h
 *
 * Note that the last 3 args (display_value, blink, brightness) is
 * expected to be set by the client on every call.  Leaving it unset
 * will produce undefined results.
 *
 * for the LEDs, the only valid display_type is int.  The low 8 bits
 * are for red, next 8 for blue, third 8 green.
 */
typedef display_type_t (*f_get_display)(struct display_strategy*, display_value_t*, ht16k33blink_t*, ht16k33brightness_t*);

/** To utilize the strategy, one must also construct a struct
 * display_strategy that gets passed to the view (and really...*this* is
 * the view.  The other stuff thinking it's the view is lower level
 * hardware mumbo jumbo stuff).
 */
struct display_strategy {
  void *userdata;
  f_get_display get_green_display;
  ht16k33blink_t green_blink;
  ht16k33brightness_t green_brightness;

  f_get_display get_blue_display;
  ht16k33blink_t blue_blink;
  ht16k33brightness_t blue_brightness;

  f_get_display get_red_display;
  ht16k33blink_t red_blink;
  ht16k33brightness_t red_brightness;

  f_get_display get_leds_display;
  ht16k33blink_t leds_blink;
  ht16k33brightness_t leds_brightness;

};



/** show the displays
 * update all 3 displays, the rows of LEDs.
 * This function may be called every ~10ms.
 * @deprecated
 * This way of getting data to the display is backasswards.  It has
 * the view pull it in via 4 different function pointers.  That alone
 * should make one want to run away.
 */
void update_displays(struct ut3k_view*, struct display_strategy*, uint32_t clock);

#endif
