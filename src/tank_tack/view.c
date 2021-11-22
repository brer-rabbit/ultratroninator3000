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
#include "view.h"

struct radarsweep {
  int state;
  int timer;
  int delay;
};

struct led_lightshow {
  int static_leds[3];
  int flash_leds[3];
  int timer;
  int delay;
};

struct view {
  struct ut3k_view *ut3k_view;
  struct ut3k_display ut3k_display;
  struct radarsweep radarsweep;
  struct led_lightshow leds;
};

static void f_radarsweep0(struct display *display, uint32_t clock);
static void f_radarsweep1(struct display *display, uint32_t clock);
static void f_radarsweep2(struct display *display, uint32_t clock);
static void f_ledlightshow(struct display *display, uint32_t clock);


struct view* create_pong_view(struct ut3k_view *ut3k_view) {
  struct view *this = (struct view*)malloc(sizeof(struct view));
  this->ut3k_view = ut3k_view;
  reset_ut3k_display(&this->ut3k_display);

  return this;
}


void free_pong_view(struct view *this) {
  free(this);
}




void set_attract(struct view *this, void *scroller, f_animator animation) {
  this->ut3k_display.displays[0].f_animate = animation;
  this->ut3k_display.displays[0].userdata = scroller;
}


void set_radarview(struct view *this) {
  this->radarsweep.state = 0;
  this->radarsweep.delay = 20;
  this->radarsweep.timer = this->radarsweep.delay;

  this->leds.static_leds[0] = 0x7;
  this->leds.static_leds[1] = 0x3F;
  this->leds.static_leds[2] = 0x0F;
  this->leds.flash_leds[0] = 0x38;
  this->leds.flash_leds[1] = 0xC0;
  this->leds.flash_leds[2] = 0x10;
  
  this->leds.delay = 50;
  this->leds.timer = this->leds.timer;

  this->ut3k_display.displays[0].f_animate = f_radarsweep0;
  this->ut3k_display.displays[0].userdata = &this->radarsweep;
  this->ut3k_display.displays[1].f_animate = f_radarsweep1;
  this->ut3k_display.displays[1].userdata = &this->radarsweep;
  this->ut3k_display.displays[2].f_animate = f_radarsweep2;
  this->ut3k_display.displays[2].userdata = &this->radarsweep;
  
  this->ut3k_display.displays[0].brightness = HT16K33_BRIGHTNESS_10;
  this->ut3k_display.displays[1].brightness = HT16K33_BRIGHTNESS_10;
  this->ut3k_display.displays[2].brightness = HT16K33_BRIGHTNESS_10;

  this->ut3k_display.leds.f_animate = f_ledlightshow;
  this->ut3k_display.leds.userdata = &this->leds;
}


void render_display(struct view *this, uint32_t clock) {
  commit_ut3k_display(this->ut3k_view, &this->ut3k_display, clock);
  // wipe after render
  clear_ut3k_display(&this->ut3k_display);

  // why here? so we don't do this in every f_radarsweepN I guess
  if (this->radarsweep.timer-- == 0) {
    this->radarsweep.timer = this->radarsweep.delay;
    this->radarsweep.state = this->radarsweep.state == 7 ? 0 : this->radarsweep.state + 1;
  }
  if (this->leds.timer-- == 0) {
    this->leds.timer = this->leds.delay;
  }

}



// radar sweep for green display
// radar sweep appears to rotate around the display, also
// give a slight fading phosphor effect.  not really, but trying.
static void f_radarsweep0(struct display *display, uint32_t clock) {
  struct radarsweep *radarsweep = (struct radarsweep*) display->userdata;

  switch (radarsweep->state) {
  case 0:
    display->display_value.display_glyph[2] |= (SEG_F | SEG_E);
    // ghost effects on previous state:
    if (radarsweep->timer == radarsweep->delay - 2 ||
        radarsweep->timer == radarsweep->delay - 5 ||
        radarsweep->timer == radarsweep->delay - 9) {
      display->display_value.display_glyph[0] |= (SEG_H | SEG_N);
    }
    break;
  case 7:
    display->display_value.display_glyph[0] |= (SEG_H | SEG_N);
    break;
  case 2:
    if (radarsweep->timer == radarsweep->delay - 2 ||
        radarsweep->timer == radarsweep->delay - 5 ||
        radarsweep->timer == radarsweep->delay - 9) {
      display->display_value.display_glyph[3] |= (SEG_L | SEG_K);
    }
    break;
  case 1:
    display->display_value.display_glyph[3] |= (SEG_L | SEG_K);
    if (radarsweep->timer == radarsweep->delay - 2 ||
        radarsweep->timer == radarsweep->delay - 5 ||
        radarsweep->timer == radarsweep->delay - 9) {
      display->display_value.display_glyph[2] |= (SEG_F | SEG_E);
    }
    break;
  default:
    break;
  }
}

static void f_radarsweep1(struct display *display, uint32_t clock) {
  struct radarsweep *radarsweep = (struct radarsweep*) display->userdata;

  switch (radarsweep->state) {
  case 6:
    display->display_value.display_glyph[0] |= (SEG_G1 | SEG_G2);
    display->display_value.display_glyph[1] |= (SEG_G1 | SEG_G2);
    break;
  case 7:
    if (radarsweep->timer == radarsweep->delay - 2 ||
        radarsweep->timer == radarsweep->delay - 5 ||
        radarsweep->timer == radarsweep->delay - 9) {
      display->display_value.display_glyph[0] |= (SEG_G1 | SEG_G2);
      display->display_value.display_glyph[1] |= (SEG_G1 | SEG_G2);
    }
    break;
  case 2:
    display->display_value.display_glyph[2] |= (SEG_G1 | SEG_G2);
    display->display_value.display_glyph[3] |= (SEG_G1 | SEG_G2);
    break;
  case 3:
    if (radarsweep->timer == radarsweep->delay - 2 ||
        radarsweep->timer == radarsweep->delay - 5 ||
        radarsweep->timer == radarsweep->delay - 9) {
      display->display_value.display_glyph[2] |= (SEG_G1 | SEG_G2);
      display->display_value.display_glyph[3] |= (SEG_G1 | SEG_G2);
    }
    break;
  default:
    break;
  }
}

static void f_radarsweep2(struct display *display, uint32_t clock) {
  struct radarsweep *radarsweep = (struct radarsweep*) display->userdata;

  switch (radarsweep->state) {
  case 5:
    display->display_value.display_glyph[0] |= (SEG_K | SEG_L);
    if (radarsweep->timer == radarsweep->delay - 2 ||
        radarsweep->timer == radarsweep->delay - 5 ||
        radarsweep->timer == radarsweep->delay - 9) {
      display->display_value.display_glyph[2] |= (SEG_F | SEG_E);
    }
    break;
  case 4:
    display->display_value.display_glyph[2] |= (SEG_F | SEG_E);
    if (radarsweep->timer == radarsweep->delay - 2 ||
        radarsweep->timer == radarsweep->delay - 5 ||
        radarsweep->timer == radarsweep->delay - 9) {
      display->display_value.display_glyph[3] |= (SEG_H | SEG_N);
    }
    break;
  case 3:
    display->display_value.display_glyph[3] |= (SEG_H | SEG_N);
    break;
  case 6:
    if (radarsweep->timer == radarsweep->delay - 2 ||
        radarsweep->timer == radarsweep->delay - 5 ||
        radarsweep->timer == radarsweep->delay - 9) {
      display->display_value.display_glyph[0] |= (SEG_K | SEG_L);
    }
    break;
  default:
    break;
  }
}



static void f_ledlightshow(struct display *display, uint32_t clock) {
  struct led_lightshow *leds = (struct led_lightshow*) display->userdata;
  int effect;
  effect = (leds->delay - leds->timer) / 7;

  if (2 * leds->timer < leds->delay) {
    set_green_leds(display, leds->static_leds[0] | leds->flash_leds[0]);
  }
  else {
    set_green_leds(display, leds->static_leds[0]);
  }

  set_blue_leds(display, leds->static_leds[1]);

  set_red_leds(display, leds->static_leds[2] ^ ( 1 << effect));
}
