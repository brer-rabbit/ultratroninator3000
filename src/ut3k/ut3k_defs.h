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
/* for viewing (this *is* actually the view, despite the file
   ut3k_view thinking it is), provide an interface the model can use to
 * set the 4-digit display.  The displayable types are:
 * integer : show an integer value, right aligned
 * string  : display a string, left aligned
 * glyph   : user provided uint16_t[4] to display, roll yer own!
 */

#ifndef UT3K_DEFS_H
#define UT3K_DEFS_H

#include <stdint.h>

typedef enum { integer_display, string_display, glyph_display } display_type_t;
typedef union {
  int32_t display_int;  // for the LED display the first 3 bytes are read
  char* display_string;
  uint16_t display_glyph[4];
} display_value_t;



#endif
