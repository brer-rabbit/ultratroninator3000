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


/* Segments:
 *       A                     ROW0 -  A    - 0000000000000001(0X0001)
 *     _____                   ROW1 -  B    - 0000000000000010(0X0002)
 *    |\H|J/|                  ROW2 -  C    - 0000000000000100(0X0004)
 *  F | \|/K| B                ROW3 -  D    - 0000000000001000(0X0008)
 *    |G1|G2|                  ROW4 -  E    - 0000000000010000(0X0010)
 *    |-- --|                  ROW5 -  F    - 0000000000100000(0X0020)
 *    | /|\N|                  ROW6 -  G1   - 0000000001000000(0X0040)
 *  E |/L|M\| C                ROW7 -  G2   - 0000000010000000(0X0080)
 *    |__|__|   . DP           ROW8 -  H    - 0000000100000000(0X0100)
 *       D                     ROW9 -  J    - 0000001000000000(0X0200)
 *                             ROW10 - K    - 0000010000000000(0X0400)
 *                             ROW11 - L    - 0000100000000000(0X0800)
 *                             ROW12 - M    - 0001000000000000(0X1000)
 *                             ROW13 - N    - 0010000000000000(0X2000)
 *                             ROW14 - DP   - 0100000000000000(0X4000)
 */

#define SEG_A 0x0001
#define SEG_B 0x0002
#define SEG_C 0x0004
#define SEG_D 0x0008
#define SEG_E 0x0010
#define SEG_F 0x0020
#define SEG_G1 0x0040
#define SEG_G2 0x0080
#define SEG_H 0x0100
#define SEG_J 0x0200
#define SEG_K 0x0400
#define SEG_L 0x0800
#define SEG_M 0x1000
#define SEG_N 0x2000
#define SEG_DP 0x4000


#endif
