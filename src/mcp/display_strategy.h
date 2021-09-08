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

/* for viewing, provide an interface the (presumably model) can use to
 * set the 4-digit display.  The displayable types are:
 * integer : show an integer value, right aligned
 * string  : display a string, left aligned
 * glyph   : user provided uint16_t[4] to display, roll yer own!
 */

typedef enum { integer_display, string_display, glyph_display } display_type;
typedef union {
  int display_int;
  char* display_string;
  uint16_t display_glyph[4];
} display_value;
    

struct display_strategy;

/** methods to implement to realize the strategy for getting a
 * particular display:
 * User of this interface will implement three
 * f_get_display types, one for each display.
 */
typedef display_type (*f_get_display)(struct display_strategy*, display_value*);

/** To utilize the strategy, one must also construct a struct display_strategy
 * that gets passed to the view.
 */
struct display_strategy {
  void *userdata;
  f_get_display get_green_display;
  f_get_display get_blue_display;
  f_get_display get_red_display;
};


#endif
