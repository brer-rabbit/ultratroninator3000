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


static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness);



/* levelup interlude: red display shows "SUPERZAPPER RECHARGE" while
 * other two displays give a light show.  start with lines diagonal upside
 * down V, transition to flatline, then go to a diagonal V display.
 */
#define DIAG_LL_UR 0x0C00
#define DIAG_LR_UL 0x2100
#define FLATLINE_LOW 0x0008
#define FLATLINE_MID 0x00C0
#define FLATLINE_HIGH 0x0001

struct display_frame {
  uint16_t glyph_green[4];
  uint16_t glyph_blue[4];
};

static const struct display_frame diag_rising0 =
  {
   .glyph_green = { 0, 0, 0, 0 },
   .glyph_blue = { 0, DIAG_LL_UR, DIAG_LR_UL, 0 }
  };
static const struct display_frame diag_rising1 =
  {
   .glyph_green = { 0, DIAG_LL_UR, DIAG_LR_UL, 0 },
   .glyph_blue = { DIAG_LL_UR, 0, 0, DIAG_LR_UL }
  };
static const struct display_frame diag_rising2 =
  {
   .glyph_green = { DIAG_LL_UR, 0, 0, DIAG_LR_UL },
   .glyph_blue = { 0, 0, 0, 0 }
  };

static const struct display_frame diag_rising[] =
  { diag_rising0, diag_rising1, diag_rising2 };



static const struct display_frame flatline0 =
  {
   .glyph_green = { FLATLINE_LOW, FLATLINE_LOW, FLATLINE_LOW, FLATLINE_LOW },
   .glyph_blue = { FLATLINE_LOW, FLATLINE_LOW, FLATLINE_LOW, FLATLINE_LOW }
  };
static const struct display_frame flatline1 =
  {
   .glyph_green = { FLATLINE_HIGH, FLATLINE_HIGH, FLATLINE_HIGH, FLATLINE_HIGH },
   .glyph_blue = { FLATLINE_MID, FLATLINE_MID, FLATLINE_MID, FLATLINE_MID }
  };
static const struct display_frame flatline2 =
  {
   .glyph_green = { 0, 0, 0, 0 },
   .glyph_blue = { FLATLINE_HIGH, FLATLINE_HIGH, FLATLINE_HIGH, FLATLINE_HIGH }
  };
static const struct display_frame flatline[] =
  { flatline0, flatline1, flatline2 };



static const struct display_frame diag_falling0 =
  {
   .glyph_green = { 0, DIAG_LR_UL, DIAG_LL_UR, 0 },
   .glyph_blue = { 0, 0, 0, 0 }
  };
static const struct display_frame diag_falling1 =
  {
   .glyph_green = { DIAG_LR_UL, 0, 0, DIAG_LL_UR },
   .glyph_blue = { 0, DIAG_LR_UL, DIAG_LL_UR, 0 }
  };
static const struct display_frame diag_falling2 =
  {
   .glyph_green = { 0, 0, 0, 0 },
   .glyph_blue = { DIAG_LR_UL, 0, 0, DIAG_LL_UR }
  };

static const struct display_frame diag_falling[] =
  { diag_falling0, diag_falling1, diag_falling2 };





// view methods

/* view methods ----------------------------------------------------- */



struct view_data {
  struct model *model;
  struct display_frame const *green_display;
  struct display_frame const *blue_display;
};

struct display_strategy* create_levelup_display_strategy(struct model *model) {
  struct display_strategy *display_strategy;
  struct view_data *view_data = (struct view_data*)malloc(sizeof(struct view_data));
  *view_data = (struct view_data const)
    {
     .model = model,
     .green_display = NULL,
     .blue_display = NULL
    };

  display_strategy = (struct display_strategy*)malloc(sizeof(struct display_strategy));
  *display_strategy = (struct display_strategy const)
    {
     .userdata = view_data,
     .get_green_display = get_green_display,
     .green_blink = HT16K33_BLINK_OFF,
     .green_brightness = HT16K33_BRIGHTNESS_5,
     .get_blue_display = get_blue_display,
     .blue_blink = HT16K33_BLINK_OFF,
     .blue_brightness = HT16K33_BRIGHTNESS_5,
     .get_red_display = get_red_display,
     .red_blink = HT16K33_BLINK_OFF,
     .red_brightness = HT16K33_BRIGHTNESS_5,
     .get_leds_display = get_leds_display,
     .leds_blink = HT16K33_BLINK_OFF,
     .leds_brightness = HT16K33_BRIGHTNESS_5
    };

  return display_strategy;
}

void free_levelup_display_strategy(struct display_strategy *display_strategy) {
  struct view_data *view_data;
  view_data = (struct view_data*) display_strategy->userdata;
  free(view_data);
  free(display_strategy);
}






// implements f_get_display for the red display
static display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct view_data *view_data = (struct view_data*) display_strategy->userdata;
  struct model *model = view_data->model;
  const struct levelup *levelup = get_model_levelup(model);

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_5;
  (*value).display_string = levelup->msg_ptr;

  return string_display;
}

// implements f_get_display for the blue display
static display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct view_data *view_data = (struct view_data*) display_strategy->userdata;
  struct model *model = view_data->model;
  const struct levelup *levelup = get_model_levelup(model);


  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_5;

  if (levelup->animation_timer == 1) {
    // go to the next frame in sequence.  Rollback to first if at end of array.
    switch (levelup->animation_state) {
    case DIAG_RISING_SLOW:
    case DIAG_RISING_MED:
    case DIAG_RISING_FAST:
      view_data->blue_display =
        (view_data->blue_display == (struct display_frame*)(&diag_rising + 1) - 1) ?
        &diag_rising[0] : view_data->blue_display + 1;
      break;
    case FLATLINE:
      view_data->blue_display =
        (view_data->blue_display == (struct display_frame*)(&flatline + 1) - 1) ?
        &flatline[0] : view_data->blue_display + 1;
      break;
    case DIAG_FALLING_SLOW:
    case DIAG_FALLING_MED:
    case DIAG_FALLING_FAST:
      view_data->blue_display =
        (view_data->blue_display == (struct display_frame*)(&diag_falling + 1) - 1) ?
        &diag_falling[0] : view_data->blue_display + 1;
      break;
    default:
      printf("how did I get here?\n");
    }
  }


  (*value).display_glyph[0] = view_data->blue_display->glyph_blue[0];
  (*value).display_glyph[1] = view_data->blue_display->glyph_blue[1];
  (*value).display_glyph[2] = view_data->blue_display->glyph_blue[2];
  (*value).display_glyph[3] = view_data->blue_display->glyph_blue[3];

  return glyph_display;
}


// implements f_get_display for the green display
static display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct view_data *view_data = (struct view_data*) display_strategy->userdata;
  struct model *model = view_data->model;
  const struct levelup *levelup = get_model_levelup(model);


  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_5;

  // poor: handling of state is in the green display, since this is called
  // first.  Better: create some actual instance methods on the view; not
  // sure yet how to do that without a hard coupling to the model.
  if (levelup->init_levelup_display == 1) {
    switch (levelup->animation_state) {
    case DIAG_RISING_SLOW:
    default:
      view_data->green_display = &diag_rising[0];
      view_data->blue_display = &diag_rising[0];
      break;
    }
    printf("init levelup display\n");
  }


  if (levelup->animation_timer == 1) {
    // go to the next frame in sequence.  Rollback to first if at end of array.
    switch (levelup->animation_state) {
    case DIAG_RISING_SLOW:
    case DIAG_RISING_MED:
    case DIAG_RISING_FAST:
      view_data->green_display =
        (view_data->green_display == (struct display_frame*)(&diag_rising + 1) - 1) ?
        &diag_rising[0] : view_data->green_display + 1;
      break;
    case FLATLINE:
      view_data->green_display =
        (view_data->green_display == (struct display_frame*)(&flatline + 1) - 1) ?
        &flatline[0] : view_data->green_display + 1;
      break;
    case DIAG_FALLING_SLOW:
    case DIAG_FALLING_MED:
    case DIAG_FALLING_FAST:
      view_data->green_display =
        (view_data->green_display == (struct display_frame*)(&diag_falling + 1) - 1) ?
        &diag_falling[0] : view_data->green_display + 1;
      break;
    default:
      printf("how did I get here?\n");
    }
  }


  (*value).display_glyph[0] = view_data->green_display->glyph_green[0];
  (*value).display_glyph[1] = view_data->green_display->glyph_green[1];
  (*value).display_glyph[2] = view_data->green_display->glyph_green[2];
  (*value).display_glyph[3] = view_data->green_display->glyph_green[3];

  return glyph_display;
}




// implements f_get_display for the leds display
static display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct view_data *view_data = (struct view_data*) display_strategy->userdata;
  struct model *model = view_data->model;

  (*value).display_int = 0;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}


