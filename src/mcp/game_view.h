/* game_view.h
 *
 */

#ifndef GAME_VIEW_H
#define GAME_VIEW_H

struct game_view;

#include "game_model.h"
#include "ht16k33.h"


typedef void (*f_view_displays)(struct game_view*, struct game_model*);
typedef void (*f_view_leds)(struct game_view*, struct game_model*);


/** initialize_game_view
 *
 * start the view off with a particular view opens HT16K33.
 * 
 */
struct game_view* create_alphanum_game_view();


/** free resources from game view
 */
int free_game_view(struct game_view *this);


/** show the view of the model-
 * update all 3 displays and any other visible components
 * TBD: should this be responsible for reading as well? likely...
 */
void update_view(struct game_view*, struct game_model*);


/* Listeners / callback */


/** callback for rotary encoder if something happened.
 * by something- either rotation or button push.  Callback provide:
 * delta (-1, 0, 1) for direction of change
 * 0/1 button state is not pushed / pushed
 * 0/1 if the button changed state
 * user data in a void* pointer
 */
typedef void (*f_controller_update_rotary_encoder)(int8_t delta, uint8_t button_pushed, uint8_t button_changed, void*);

 /**
  * Register event handlers with the appropriate components
  */
void register_green_encoder_listener(struct game_view *view, f_controller_update_rotary_encoder f, void *userdata);
 


#endif
