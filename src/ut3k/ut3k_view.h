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

#ifndef UT3K_VIEW_H
#define UT3K_VIEW_H

#include "ut3k_defs.h"
#include "ht16k33.h"
#include "control_panel.h"


struct ut3k_view;
struct display;



/** create_alphanum_ut3k_view
 *
 * "constructor"
 * start the view off with a particular view opens HT16K33.
 * This does a read from the HT16K33: the caller must wait >~20 ms
 * before any other reads against this chip.  Such as update_view().
 *
 */
struct ut3k_view* create_alphanum_ut3k_view();


/** free resources from game view
 * well technically, all games are set to free play here...
 */
int free_ut3k_view(struct ut3k_view *this);


/** 
 * Keyscan all controls.
 * Keyscan results are available via the callback registered listener
 * or via the get_control_panel call.
 * This function must only be called not more than every ~20ms,
 * otherwise the caller risks getting invalid results from the chip.
 */
void update_controls(struct ut3k_view*, uint32_t clock);


 /**
  * Register event handlers with the appropriate components.
  * The control_panel_listener will be called during update_view.
  */
typedef void (*f_view_control_panel_listener)(const struct control_panel *control_panel, void *userdata);
void register_control_panel_listener(struct ut3k_view *view, f_view_control_panel_listener f, void *userdata);


/** get the control panel.  Useful for initialization or if you really
 *   want to avoid the callback model.
 */
const struct control_panel* get_control_panel(struct ut3k_view*);


typedef void (*f_animator)(struct display*, uint32_t clock);

/***
 * struct ut3k_display and it's underlying struct display are the
 * display buffers the client has to work with.  Update and modify these
 * to get something usable out of this infernal machine.  Method
 * commit_ut3k_display will write the buffer to the display and, between
 * cycles, clear_ut3k_display to get a clean slate.
 * How this is better than the previous way I had I dunno.
 */

struct display {
  // a display has a type and a value.  It can blink, it has a brightness
  // level.  It may, if not null, be manipulated by an animator.  The
  // animator is passed userdata.
  // Animator limitation: the f_animator only has access to this single
  // display.  If something should go vertical, then that'll require
  // some other coordination.  Figure it out external to this...
  // Tried not to call this an ht16k33_display, it sits ever so
  // slightly higher than that.  ok, maybe not.  anyway, it translates
  // the types to something for the ht16k33 library.
  display_type_t display_type;
  display_value_t display_value;
  ht16k33blink_t blink;
  ht16k33brightness_t brightness;
  f_animator f_animate;
  void *userdata;  // may be a struct text_scroller, but that's not enforced...
};


// The whole enchilada of the displays.  This is the thing.
struct ut3k_display {
  struct display displays[3];
  struct display leds;
};


/** commit_ut3k_view
 *
 * this method take a ut3k_view and a pointer to the entire display, a
 * ut3k_display, along with the clock.  Commits the ut3k_display buffer,
 * writing it to HT16K33s.
 */
void commit_ut3k_view(struct ut3k_view *this, struct ut3k_display *ut3k_display, uint32_t clock);

// convenience function - clears the display buffer only, no
// write/commit is involved.  so one can start a display cycle with a
// clean slate.
// This does _not_ change brightness, blink, f_animate, userdata values.
void clear_ut3k_display(struct ut3k_display*);

// As per clear above, but do clear blink (Off), reset brightness (7),
// clear/null f_animate and userdata.
void reset_ut3k_display(struct ut3k_display*);

void set_green_leds(struct display*, uint16_t);
void set_blue_leds(struct display*, uint16_t);
void set_red_leds(struct display*, uint16_t);


///// f_animator functionality

// "base" class
struct text_scroller {
  const char *text;
  char *position;
  int scroll_completed;
};

void init_text_scroller(struct text_scroller *scroller, const char *text);
void text_scroller_forward(struct text_scroller *scroller);
void text_scroller_backward(struct text_scroller *scroller);
int text_scroller_is_complete(struct text_scroller *scroller);
void text_scroller_reset(struct text_scroller *scroller);


// derived/decorated/whatevz.  Do more things than the base-class-class.
// These are intended to be used- the text_scroller not so much.

struct clock_text_scroller {
  struct text_scroller scroller_base;
  uint32_t timer;
  uint32_t delay;
};

struct manual_text_scroller {
  struct text_scroller scroller_base;
  int direction;
};


void f_clock_text_scroller(struct display *display, uint32_t clock);
void f_manual_text_scroller(struct display *display, uint32_t clock);

void init_clock_text_scroller(struct clock_text_scroller *scroller, const char *text, int timer);
void init_manual_text_scroller(struct manual_text_scroller *scroller, const char *text);


#endif
