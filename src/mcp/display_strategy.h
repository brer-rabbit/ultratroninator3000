/* display interace for decoupling view & model --
 * this is somewhere between a strategy and a bridge...
 * allow for decoupling of the model from the view, and
 * at the same time provide the model with flexiblity to
 * display a variety of different data
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

// method to implement in the strategy for getting a particular display
typedef display_type (*f_get_display)(struct display_strategy*, display_value*);

struct display_strategy {
  void *userdata;
  f_get_display get_green_display;
  f_get_display get_blue_display;
  f_get_display get_red_display;
};


#endif
