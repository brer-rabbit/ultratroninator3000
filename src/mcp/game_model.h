/* game_model.h
 *
 * define the model
 */

#ifndef GAME_MODEL_H
#define GAME_MODEL_H


struct game_model;

/* init model.  Caller is responsible for allocating game_model memory */
struct game_model* create_game_model();
void free_game_model(struct game_model*);

int set_green_string(struct game_model *this, char *display_string);
int set_blue_string(struct game_model *this, char *display_string);
int set_red_string(struct game_model *this, char *display_string);

char* get_green_string(struct game_model *this);
char* get_blue_string(struct game_model *this);
char* get_red_string(struct game_model *this);



#endif
