#ifndef HT16K33_LOOKUP_TABLES_H
#define HT16K33_LOOKUP_TABLES_H

#include <stdint.h>

extern const uint16_t ht16k33_int_to_display[256][3];
extern const uint16_t ht16k33_alphanum[];
extern const uint8_t ht16k33_7seg_digits[];


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

extern const uint16_t seg_A;
extern const uint16_t seg_B;
extern const uint16_t seg_C;
extern const uint16_t seg_D;
extern const uint16_t seg_E;
extern const uint16_t seg_F;
extern const uint16_t seg_G1;
extern const uint16_t seg_G2;
extern const uint16_t seg_H;
extern const uint16_t seg_J;
extern const uint16_t seg_K;
extern const uint16_t seg_L;
extern const uint16_t seg_M;
extern const uint16_t seg_N;
extern const uint16_t seg_DP;



#endif
