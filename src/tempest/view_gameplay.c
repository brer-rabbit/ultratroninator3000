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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "tempest.h"
#include "model.h"
#include "view_gameplay.h"

static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);


// view methods
typedef enum { DISPLAY_RED, DISPLAY_BLUE, DISPLAY_GREEN } display_t;
static void display_flipper(const struct flipper*, display_t, display_value*);
static void display_blaster(const struct blaster*, display_t, display_value*);

/* view methods ----------------------------------------------------- */


struct display_strategy* create_gameplay_display_strategy(struct model *model) {
  struct display_strategy *display_strategy;
  display_strategy = (struct display_strategy*)malloc(sizeof(struct display_strategy));
  display_strategy->userdata = (void*)model;
  display_strategy->get_green_display = get_green_display;
  display_strategy->green_blink = HT16K33_BLINK_OFF;
  display_strategy->green_brightness = HT16K33_BRIGHTNESS_12;
  display_strategy->get_blue_display = get_blue_display;
  display_strategy->blue_blink = HT16K33_BLINK_OFF;
  display_strategy->blue_brightness = HT16K33_BRIGHTNESS_12;
  display_strategy->get_red_display = get_red_display;
  display_strategy->red_blink = HT16K33_BLINK_OFF;
  display_strategy->red_brightness = HT16K33_BRIGHTNESS_12;
  display_strategy->get_leds_display = get_leds_display;
  display_strategy->leds_blink = HT16K33_BLINK_OFF;
  display_strategy->leds_brightness = HT16K33_BRIGHTNESS_12;
  return display_strategy;
}




static const uint16_t player_glyph[] =
  {
   0x0850, 0x0160, 0x0850, 0x1800, 0x3000, 0x1800, 0x3000, 0x1800, 0x3000,
   0x1800, 0x3000, 0x2084, 0x0482, 0x2084, 0x0482, 0x2084, 0x0482, 0x0600,
   0x0300, 0x0600, 0x0300, 0x0600, 0x0300, 0x0600, 0x0300, 0x0160, 0x0850,
   0x0160
  };


// implements f_get_display for the red display
static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct player *player = get_model_player(model);
  const struct playfield *playfield = get_model_playfield(model);

  *brightness = HT16K33_BRIGHTNESS_12;
  *blink = HT16K33_BLINK_OFF;

  memset((*value).display_glyph, 0, sizeof(display_value));

  if (player->position != 0) {
    if (player->position > 0 && player->position < 5) {
      (*value).display_glyph[0] = player_glyph[player->position];
    }
    else if (player->position < 7) {
      (*value).display_glyph[1] = player_glyph[player->position];
    }  
    else if (player->position < 9) {
      (*value).display_glyph[2] = player_glyph[player->position];
    }  
    else if (player->position < 13) {
      (*value).display_glyph[3] = player_glyph[player->position];
    }  
  }

  // draw any active flippers
  for (int i = 0; i < playfield->num_flippers; ++i) {
    display_flipper(&playfield->flippers[i], DISPLAY_RED, value);
  }

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    const struct blaster *blaster = &player->blaster[i];
    if (blaster->blaster_state == FIRED) {
      display_blaster(blaster, DISPLAY_RED, value);
    }
  }

  return glyph_display;
}

// implements f_get_display for the blue display
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct player *player = get_model_player(model);
  const struct playfield *playfield = get_model_playfield(model);


  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  memset((*value).display_glyph, 0, sizeof(display_value));

  if (player->position == 0 || player->position == 27) {
    (*value).display_glyph[0] = player_glyph[player->position];
  }
  else if (player->position == 13 || player->position == 14) {
    (*value).display_glyph[3] = player_glyph[player->position];
  }

  // draw any active flippers
  for (int i = 0; i < playfield->num_flippers; ++i) {
    display_flipper(&playfield->flippers[i], DISPLAY_BLUE, value);
  }

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    const struct blaster *blaster = &player->blaster[i];
    if (blaster->blaster_state == FIRED) {
      display_blaster(blaster, DISPLAY_BLUE, value);
    }
  }

  return glyph_display;
}

// implements f_get_display for the green display
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct player *player = get_model_player(model);
  const struct playfield *playfield = get_model_playfield(model);


  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  memset((*value).display_glyph, 0, sizeof(display_value));

  if (player->position != 27) {
    if (player->position > 22) {
      (*value).display_glyph[0] = player_glyph[player->position];
    }
    else if (player->position > 20) {
      (*value).display_glyph[1] = player_glyph[player->position];
    }
    else if (player->position > 18) {
      (*value).display_glyph[2] = player_glyph[player->position];
    }
    else if (player->position > 14) {
      (*value).display_glyph[3] = player_glyph[player->position];
    }
  }

  // draw any active flippers
  for (int i = 0; i < playfield->num_flippers; ++i) {
    display_flipper(&playfield->flippers[i], DISPLAY_GREEN, value);
  }

  for (int i = 0; i < MAX_BLASTER_SHOTS; ++i) {
    const struct blaster *blaster = &player->blaster[i];
    if (blaster->blaster_state == FIRED) {
      display_blaster(blaster, DISPLAY_GREEN, value);
    }
  }

  return glyph_display;
}




// implements f_get_display for the leds display
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *model = (struct model*) display_strategy->userdata;
  const struct playfield *playfield = get_model_playfield(model);

  (*value).display_int = 0;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  (*value).display_int = 0;

  for (int i = 0; i < playfield->num_flippers; ++i) {
    if (playfield->flippers[i].flipper_state == SPAWNING) {
      (*value).display_int |= ((1 << playfield->flippers[i].position) << 8);
    }
  }


  return integer_display;
}





// this completely convoluted data structure starts at the center of
// the playfield, each list starts at 9 o'clock and goes CCW around
// from there.

// this is intended to handle blaster, flippers, and maybe other stuff
// that gets created...and it's a mess.
struct object_position_to_display_digit_and_glyph {
  display_t display;
  int digit;
  uint16_t glyph_active;
  uint16_t glyph_move[3];
};

static const struct object_position_to_display_digit_and_glyph blaster_glyph_by_position_d1[] =
  {
   { DISPLAY_BLUE, 0, 0x4000, { } },
   { DISPLAY_RED,  0, 0x0400, { } },
   { DISPLAY_RED,  0, 0x0400, { } },
   { DISPLAY_RED,  1, 0x0020, { } },
   { DISPLAY_RED,  1, 0x0002, { } },
   { DISPLAY_RED,  2, 0x0200, { } },
   { DISPLAY_RED,  3, 0x0020, { } },
   { DISPLAY_RED,  3, 0x0100, { } },
   { DISPLAY_RED,  3, 0x0001, { } },
   { DISPLAY_BLUE, 3, 0x0008, { } },
   { DISPLAY_BLUE, 3, 0x0001, { } },
   { DISPLAY_GREEN,3, 0x0800, { } },
   { DISPLAY_GREEN,3, 0x0010, { } },
   { DISPLAY_GREEN,2, 0x0004, { } },
   { DISPLAY_GREEN,2, 0x0010, { } },
   { DISPLAY_GREEN,1, 0x1000, { } },
   { DISPLAY_GREEN,0, 0x0004, { } },
   { DISPLAY_GREEN,0, 0x2000, { } },
   { DISPLAY_GREEN,0, 0x0008, { } },
   { DISPLAY_BLUE, 0, 0x0080, { } }
  };

static const struct object_position_to_display_digit_and_glyph blaster_glyph_by_position_d3[] =
  {
   { DISPLAY_BLUE, 1, 0x0004, { } },
   { DISPLAY_BLUE, 2, 0x0010, { } },
   { DISPLAY_BLUE, 2, 0x0020, { } },
   { DISPLAY_BLUE, 1, 0x0002, { } }
  };

static const struct object_position_to_display_digit_and_glyph *blaster_glyph_by_depth_position[] =
  {
   NULL,
   blaster_glyph_by_position_d1,
   NULL,
   blaster_glyph_by_position_d3,
   NULL
  };



// lookup by flipper position for depth3 ("d3")
static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d4[] =
  {
   { DISPLAY_BLUE, 1, 0x0004, { 0x0004, 0x0004, 0x0004 } },
   { DISPLAY_BLUE, 2, 0x0010, { 0x0010, 0x0010, 0x0010 } },
   { DISPLAY_BLUE, 2, 0x0020, { 0x0020, 0x0020, 0x0020 } },
   { DISPLAY_BLUE, 1, 0x0002, { 0x0002, 0x0002, 0x0002 } }
  };


static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d3[] =
  {
   { DISPLAY_BLUE, 1, 0x1000, { 0x2000, 0x0080, 0x0004 } },
   { DISPLAY_BLUE, 2, 0x1000, { 0x0800, 0x0040, 0x0010 } },
   { DISPLAY_BLUE, 2, 0x0200, { 0x0100, 0x0040, 0x0020 } },
   { DISPLAY_BLUE, 1, 0x0200, { 0x0400, 0x0080, 0x0002 } }
  };

static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d2[] =
  // 12 elements
  {
   { DISPLAY_BLUE,  1, 0x0010, { 0x0008, 0x0800, 0x2000 } },
   { DISPLAY_RED,   1, 0x0020, { 0x0001, 0x0100, 0x0040 } },
   { DISPLAY_RED,   1, 0x0002, { 0x0001, 0x0200, 0x0400 } },
   { DISPLAY_RED,   2, 0x0020, { 0x0001, 0x0100, 0x0040 } },
   { DISPLAY_RED,   2, 0x0002, { 0x0001, 0x0200, 0x0400 } },
   { DISPLAY_BLUE,  2, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_BLUE,  2, 0x0002, { 0x0400, 0x0200, 0x0100 } },
   { DISPLAY_GREEN, 2, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_GREEN, 2, 0x0008, { 0x1000, 0x0800, 0x0040 } },
   { DISPLAY_GREEN, 1, 0x2000, { 0x0004, 0x0080, 0x0004 } },
   { DISPLAY_GREEN, 1, 0x0008, { 0x1000, 0x0800, 0x0040 } },
   { DISPLAY_BLUE,  1, 0x0020, { 0x0001, 0x0100, 0x0400 } }
  };


static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d1[] =
  // 20 elements
  {
   { DISPLAY_BLUE,  0, 0x0004, { 0x0040, 0x2000, 0x1000 } },
   { DISPLAY_RED,   0, 0x0001, { 0x0200, 0x0400, 0x0002 } },
   { DISPLAY_RED,   0, 0x0002, { 0x0080, 0x0400, 0x0200 } },
   { DISPLAY_RED,   1, 0x0020, { 0x0100, 0x0200, 0x0040 } },
   { DISPLAY_RED,   1, 0x0001, { 0x0200, 0x0400, 0x0080 } },
   { DISPLAY_RED,   2, 0x0020, { 0x0100, 0x0200, 0x0040 } },
   { DISPLAY_RED,   2, 0x0001, { 0x0200, 0x0400, 0x0080 } },
   { DISPLAY_RED,   3, 0x0020, { 0x0040, 0x0100, 0x0200 } },
   { DISPLAY_RED,   3, 0x0001, { 0x0200, 0x0100, 0x0040 } },
   { DISPLAY_BLUE,  3, 0x0010, { 0x0800, 0x1000, 0x2000 } },
   { DISPLAY_BLUE,  3, 0x0020, { 0x0100, 0x0200, 0x0400 } },
   { DISPLAY_GREEN, 3, 0x0008, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_GREEN, 3, 0x0010, { 0x0040, 0x1000, 0x0800 } },
   { DISPLAY_GREEN, 2, 0x0004, { 0x1000, 0x2000, 0x1000 } },
   { DISPLAY_GREEN, 2, 0x0008, { 0x0010, 0x0800, 0x1000 } },
   { DISPLAY_GREEN, 1, 0x0004, { 0x1000, 0x2000, 0x1000 } },
   { DISPLAY_GREEN, 1, 0x0008, { 0x0010, 0x0800, 0x1000 } },
   { DISPLAY_GREEN, 0, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_GREEN, 0, 0x0008, { 0x0800, 0x1000, 0x2000 } },
   { DISPLAY_BLUE,  0, 0x0002, { 0x0400, 0x0200, 0x0100 } }
  };


static const struct object_position_to_display_digit_and_glyph flipper_glyph_by_position_d0[] =
  // 28 elements
  {
   { DISPLAY_BLUE,  0, 0x0010, { 0x0800, 0x1000, 0x2000 } },
   { DISPLAY_RED,   0, 0x0020, { 0x0100, 0x0200, 0x0400 } },
   { DISPLAY_RED,   0, 0x0010, { 0x0040, 0x0800, 0x1000 } },
   { DISPLAY_RED,   0, 0x0008, { 0x0800, 0x0040, 0x0080 } },
   { DISPLAY_RED,   0, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_RED,   1, 0x0008, { 0x0800, 0x0040, 0x0080 } },
   { DISPLAY_RED,   1, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_RED,   2, 0x0008, { 0x0800, 0x0040, 0x0080 } },
   { DISPLAY_RED,   2, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_RED,   3, 0x0008, { 0x0800, 0x0040, 0x0080 } },
   { DISPLAY_RED,   3, 0x4000, { 0x0004, 0x0080, 0x2000 } },
   { DISPLAY_RED,   3, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_RED,   3, 0x0002, { 0x0400, 0x0200, 0x0100 } },
   { DISPLAY_BLUE,  3, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_BLUE,  3, 0x0002, { 0x0400, 0x0200, 0x0100 } },
   { DISPLAY_GREEN, 3, 0x0004, { 0x2000, 0x1000, 0x0800 } },
   { DISPLAY_GREEN, 3, 0x0002, { 0x0400, 0x0200, 0x0100 } },
   { DISPLAY_GREEN, 3, 0x0400, { 0x0001, 0x0200, 0x0080 } },
   { DISPLAY_GREEN, 3, 0x0100, { 0x0001, 0x0200, 0x0040 } },
   { DISPLAY_GREEN, 2, 0x0400, { 0x0001, 0x0200, 0x0080 } },
   { DISPLAY_GREEN, 2, 0x0100, { 0x0001, 0x0200, 0x0040 } },
   { DISPLAY_GREEN, 1, 0x0400, { 0x0001, 0x0200, 0x0080 } },
   { DISPLAY_GREEN, 1, 0x0100, { 0x0001, 0x0200, 0x0040 } },
   { DISPLAY_GREEN, 0, 0x0400, { 0x0001, 0x0200, 0x0080 } },
   { DISPLAY_GREEN, 0, 0x0100, { 0x0001, 0x0200, 0x0040 } },
   { DISPLAY_GREEN, 0, 0x0020, { 0x0100, 0x0200, 0x0400 } },
   { DISPLAY_GREEN, 0, 0x0010, { 0x0800, 0x1000, 0x2000 } },
   { DISPLAY_BLUE,  0, 0x0020, { 0x0100, 0x0200, 0x0400 } }
  };



static const struct object_position_to_display_digit_and_glyph *flipper_glyph_by_depth_position[] =
  {
   flipper_glyph_by_position_d0,
   flipper_glyph_by_position_d1,
   flipper_glyph_by_position_d2,
   flipper_glyph_by_position_d3,
   flipper_glyph_by_position_d4
  };


static void display_flipper(const struct flipper *flipper, display_t display, display_value *value) {
  int flipper_depth = flipper->depth;
  int flipper_position = flipper->position;
  int flipper_state = flipper->flipper_state;

    
  if (flipper_glyph_by_depth_position[flipper_depth][flipper_position].display == display) {
    // trying for at least a little readability from nested lookups...

    switch (flipper_state) {
    case ACTIVE:      
      (*value).display_glyph[ flipper_glyph_by_depth_position[flipper_depth][flipper_position].digit ] |=
        flipper_glyph_by_depth_position[flipper_depth][flipper_position].glyph_active;
      break;
    case FLIPPING_FROM1:
    case FLIPPING_FROM2:
    case FLIPPING_FROM3:
      (*value).display_glyph[ flipper_glyph_by_depth_position[flipper_depth][flipper_position].digit ] |=
        flipper_glyph_by_depth_position[flipper_depth][flipper_position].glyph_move[flipper_state - FLIPPING_FROM1];
      break;
    case FLIPPING_TO3:
    case FLIPPING_TO2:
    case FLIPPING_TO1:
      // reverse index the glyph_move array: start high, go low
      (*value).display_glyph[ flipper_glyph_by_depth_position[flipper_depth][flipper_position].digit ] |=
        flipper_glyph_by_depth_position[flipper_depth][flipper_position].glyph_move[-1 * (flipper_state - FLIPPING_TO1)];

    default:
      break;
    }
  }

}


static void display_blaster(const struct blaster *blaster, display_t display, display_value *value) {
  int blaster_depth = blaster->depth;
  int blaster_position = blaster->position;

    
  if (blaster_glyph_by_depth_position[blaster->depth] != NULL) {
    if (blaster_glyph_by_depth_position[blaster->depth][blaster_position].display == display) {
      (*value).display_glyph[ blaster_glyph_by_depth_position[blaster_depth][blaster_position].digit ] |=
        blaster_glyph_by_depth_position[blaster_depth][blaster_position].glyph_active;
    }
  }

}
