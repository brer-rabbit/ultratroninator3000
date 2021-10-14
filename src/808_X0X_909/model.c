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


/* initialize_model
 *
 * get the backpacks up and going. Clear their mem first.
 **/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "model.h"
#include "ut3k_pulseaudio.h"

#define STEPS_IN_SEQUENCE 16
#define NUM_TRIGGERED_INSTRUMENTS 16
#define NUM_VOLUME_LEVELS 12

#define STEP_NUM_TO_TRIGGER_BITMASK(x) (1 << x)

typedef enum { SEQUENCE_STOP, SEQUENCE_RUN } run_state_t;

static const uint32_t led_map[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
static const uint32_t step_map[] = { 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8 };


struct instrument {
  char *sample_name;
  char *display_name;
};


struct triggered_instrument {
  uint16_t trigger_bitmask;
  uint32_t volume;
  struct instrument *instrument;
};

struct model {
  struct display_strategy *display_strategy;

  // allow for 16 instruments, packed into a 2 byte structure for each 16th note.
  // Each element of the array is a step in the sequence.
  // If it's playing on this step:
  //  ( sequence[current_step] & {instr_mask} )
  // then output it
  uint16_t sequence[STEPS_IN_SEQUENCE];
  uint8_t current_step;
  struct triggered_instrument triggered_instruments[NUM_TRIGGERED_INSTRUMENTS];
  int current_triggered_instrument_index;
  uint8_t bpm;
  run_state_t run_state;

  // from: https://www.kvraudio.com/forum/viewtopic.php?t=261858
  //     Turns out that the shuffle of the TR-909 delays each
  //     even-numbered 1/16th by 2/96 of a beat for shuffle setting 1,
  //     4/96 for 2, 6/96 for 3, 8/96 for 4, 10/96 for 5 and 12/96 for 6.
  // for this we'll use three values of shuffle, or no shuffle.  
  // There is no shuffle here.  Here on the north pole...
  shuffle_t shuffle;
  uint32_t volume_levels[NUM_VOLUME_LEVELS];

  char ***sample_keys;
};



static struct display_strategy* create_display_strategy(struct model *this);
static void free_display_strategy(struct display_strategy *display_strategy); 
static int is_triggerable_step(uint8_t step, shuffle_t shuffle);


/** create the model.
 */
struct model* create_model(char ***sample_keys) {
  int max_volume = ut3k_get_default_volume();

  struct model* this = (struct model*)malloc(sizeof(struct model));
  *this = (struct model const)
    {
     .display_strategy = create_display_strategy(this),
     .sequence = { 0 },
     .current_step = 0,
     .current_triggered_instrument_index = 0,
     .bpm = 120,
     .shuffle = 0,
     .run_state = SEQUENCE_STOP,
     .sample_keys = sample_keys
    };

  // set volume levels for values.  Ten being the default volume
  // level.  Allow the volume to go to 11.
  for (int volume_level_idx = 0; volume_level_idx < NUM_VOLUME_LEVELS; ++volume_level_idx) {
    // multiply by 0.1 first to avoid overflow
    this->volume_levels[volume_level_idx] = (max_volume * 0.1) * volume_level_idx;
  }


  for (int trigger_idx = 0; trigger_idx < 16; ++trigger_idx) {
    this->triggered_instruments[trigger_idx].trigger_bitmask = 1 << trigger_idx;
    this->triggered_instruments[trigger_idx].volume = max_volume;
  }

  
  for (int instr_num = 0; instr_num < 16; ++instr_num) {
    this->triggered_instruments[instr_num].instrument =
      (struct instrument*) malloc(sizeof(struct instrument));
    this->triggered_instruments[instr_num].instrument->sample_name = sample_keys[instr_num][0];
    this->triggered_instruments[instr_num].instrument->display_name = sample_keys[instr_num][0];    
  }

  this->triggered_instruments[15].instrument =
      (struct instrument*) malloc(sizeof(struct instrument));
  this->triggered_instruments[15].instrument->sample_name = NULL;
  this->triggered_instruments[15].instrument->display_name = "ACNT";


  return this;
}


void free_model(struct model *this) {
  free_display_strategy(this->display_strategy);
  return free(this);
}




void clocktick_model(struct model *this) {
  int triggers_at_step;

  if (++this->current_step > 127) {
    this->current_step = 0;
  }

  if (this->run_state == SEQUENCE_STOP) {
    return;
  }

  // are we on a triggerable step?
  // if so, check what instruments should play on this step.
  if (is_triggerable_step(this->current_step, this->shuffle)) {
    // drop current_step down to a sixteenth note
    //triggers_at_step = this->sequence[step_map[this->current_step >> 3]];
    triggers_at_step = this->sequence[this->current_step >> 3];
    // sequence contains a bitmap of every instrument to
    // play at this step
    for (int triggered_instrument = 0; triggered_instrument < 16; ++triggered_instrument) {
      if ((triggers_at_step & (1 << triggered_instrument))) {
	printf("playing instrument %d on step %d:\n", triggered_instrument+1, this->current_step >> 3);
	if (this->triggered_instruments[triggered_instrument].instrument->sample_name != NULL) {
	  ut3k_play_sample(this->triggered_instruments[triggered_instrument].instrument->sample_name);
	}
      }
    }
  }
}


void toggle_run_state(struct model *this) {
  if (this->run_state == SEQUENCE_STOP) {
    this->run_state = SEQUENCE_RUN;
    this->current_step = 0;
  }
  else {
    this->run_state = SEQUENCE_STOP;
  }
}


void set_bpm(struct model *this, int bpm) {
  if (bpm >= 50 && bpm <= 240) {
    this->bpm = bpm;
  }
}


int get_bpm(struct model *this) {
  return this->bpm;
}

void change_bpm(struct model *this, int amount) {
  this->bpm += amount;
  if (this->bpm > 240) {
    this->bpm = 240;
  }
  else if (this->bpm < 50) {
    this->bpm = 50;
  }
}


// instrument should be a single bit set on a 16 bit int
void change_instrument(struct model *this, int amount) {

  this->current_triggered_instrument_index += amount;

  if (this->current_triggered_instrument_index < 0) {
    this->current_triggered_instrument_index = 0;
  }
  else if (this->current_triggered_instrument_index > 15) {
    this->current_triggered_instrument_index = 15;
  }

}


/** change_instrument_sample
 *
 * each instrument ought to have four samples
 */
void change_instrument_sample(struct model *this, int sample_num) {
  assert(sample_num >= 0 && sample_num < 5);

  // this is a completely horrid data structure
  this->triggered_instruments[this->current_triggered_instrument_index].instrument->sample_name = this->sample_keys[this->current_triggered_instrument_index][sample_num];
  this->triggered_instruments[this->current_triggered_instrument_index].instrument->display_name = this->sample_keys[this->current_triggered_instrument_index][sample_num];
}

void toggle_current_triggered_instrument_at_step(struct model *this, int step) {
  // turn on or off the current trigger at a specific step

  int trigger_bitmask =
    this->triggered_instruments[this->current_triggered_instrument_index].trigger_bitmask;
  this->sequence[step] = this->sequence[step] ^ trigger_bitmask;

  printf("setting instr %s at step %d: 0x%X\n",
	 this->triggered_instruments[this->current_triggered_instrument_index].instrument->sample_name,
	 step, this->sequence[step]);
}


void set_trigger_at_step(struct model *this, int trigger_idx, int step) {
  this->sequence[step] |= STEP_NUM_TO_TRIGGER_BITMASK(trigger_idx);
}

void clear_trigger_at_step(struct model *this, int trigger_idx, int step) {
  this->sequence[step] &= (~ STEP_NUM_TO_TRIGGER_BITMASK(trigger_idx) );
}


void set_shuffle(struct model *this, shuffle_t shuffle) {
  assert(shuffle >= 0 && shuffle < 4);
  this->shuffle = shuffle;
}


// implements f_get_display for the red display
display_type get_red_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  //(*value).display_int = ...

  (*value).display_string = "3";
  *brightness = HT16K33_BRIGHTNESS_12;
  *blink = HT16K33_BLINK_OFF;
  return string_display;
}

// implements f_get_display for the blue display
display_type get_blue_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  (*value).display_string = this->triggered_instruments[this->current_triggered_instrument_index].instrument->display_name;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return string_display;
}

// implements f_get_display for the green display
display_type get_green_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  (*value).display_int = this->bpm;
  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}



static const uint8_t green_led_offset = 16;
static const uint8_t blue_led_offset = 8;

// implements f_get_display for the leds display
display_type get_leds_display(struct display_strategy *display_strategy, display_value *value, ht16k33blink_t *blink, ht16k33brightness_t *brightness) {
  struct model *this = (struct model*) display_strategy->userdata;

  if (this->run_state == SEQUENCE_RUN) {
    // light one LED to show where the current step is in the 16 step sequence
    if (this->current_step < 64) {
      // start on the green display for steps 1 - 8
      (*value).display_int = led_map[(this->current_step >> 3)] << green_led_offset;
    }
    else {
      // then move to blue display for steps 9 - 16
      (*value).display_int = led_map[((this->current_step >> 3) - 8)] << blue_led_offset;
    }
  }
  else {
    // we're stopped - keep the first LED going to the beat at the first
    // green LED
    (*value).display_int = (this->current_step & 0b10000) ?
      led_map[0] << green_led_offset : 0;
  }

  uint16_t trigger_bitmask = this->triggered_instruments[this->current_triggered_instrument_index].trigger_bitmask;

  // show the triggers for the current instrument
  for (int step = 0; step < 8; ++step) {
    if ((this->sequence[step] & trigger_bitmask)) {
      (*value).display_int = (*value).display_int ^ (1 << (step_map[step] + green_led_offset));
    }
  }
  for (int step = 8; step < STEPS_IN_SEQUENCE; ++step) {
    if ((this->sequence[step] & trigger_bitmask)) {
      (*value).display_int = (*value).display_int ^ (1 << step_map[step]);
      // no blue led offset since the steps are 8-15 here
    }
  }

  // for the red LEDs:
  /*
  for (int step = 0; step < 8; ++step) {
    if ((this->sequence[step] & trigger_bitmask)) {
      (*value).display_int = (*value).display_int | (1 << step);
    }
  }
  */

  *blink = HT16K33_BLINK_OFF;
  *brightness = HT16K33_BRIGHTNESS_12;

  return integer_display;
}



struct display_strategy* get_display_strategy(struct model *this) {
  return this->display_strategy;
}



/* static ----------------------------------------------------------- */


static struct display_strategy* create_display_strategy(struct model *this) {
  struct display_strategy *display_strategy;
  display_strategy = (struct display_strategy*)malloc(sizeof(struct display_strategy));
  display_strategy->userdata = (void*)this;
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

static void free_display_strategy(struct display_strategy *display_strategy) {
  free(display_strategy);
}


/** is_triggerable_step
 *
 * given we're clocking at 128th notes, determine if this is a 16th note.
 * Further, delay the note if shuffle is set.
 */
const static uint8_t shuffle_mask[4] = { 0b1000, 0b1001, 0b1010, 0b1100 };
  
static int is_triggerable_step(uint8_t step, shuffle_t shuffle) {
  assert(shuffle >= 0 && shuffle < 4);
  // all odd 16th notes are triggerable regardless of shuffle
  // (odd --> start numbering of first note, not zeroth index you programmy type
  if ((step & 0b11110000) == step) {
    return 1;
  }

  // even steps
  if ((step & 0b00001111) == shuffle_mask[shuffle]) {
    return 1;
  }

  return 0;
}



