/* game_view.h
 *
 */

#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include "game_model.h"

#include "ht16k33.h"


struct game_view;
typedef void (*f_view_game)(struct game_view*, struct game_model*);
void display_view(struct game_view*, struct game_model*);


/** initialize_game_view
 *
 * start the view off with a particular view
 */
struct game_view* create_alphanum_game_view();


/* free resources from game view.  Does not deallocate the game_view itself */
int free_game_view(struct game_view *this);


#endif
