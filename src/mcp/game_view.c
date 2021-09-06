#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_view.h"
#include "game_model.h"

#define GREEN_DISPLAY_ADDRESS HT16K33_ADDR_07
#define BLUE_DISPLAY_ADDRESS HT16K33_ADDR_06
#define RED_DISPLAY_ADDRESS HT16K33_ADDR_05
#define I2C_ADAPTER_1 1

#define DISPLAY_GREEN 0
#define DISPLAY_BLUE 1
#define DISPLAY_RED 2
#define DISPLAY_ROWS 3


struct game_view {
  f_view_game display;

  HT16K33 *green_display;
  HT16K33 *blue_display;
  HT16K33 *red_display;

  // aliases for the above
  HT16K33 *display_array[3];
};



static int initialize_backpack(HT16K33 *backpack);
static void ht16k33_alphanum_display_game(struct game_view *this, struct game_model *model);



/** create_alphanum_game_view
 * clients will want to call this function with a game_view to construct
 * an alphanum view.
 * Creates a view that utilizes three of the Adafruit HT16K33 alphanum
 * displays
 */
struct game_view* create_alphanum_game_view() {
  struct game_view *this;
  int rc = 0;

  this = (struct game_view*) malloc(sizeof(struct game_view));

  this->display = ht16k33_alphanum_display_game;

  this->green_display = (HT16K33*) malloc(sizeof(HT16K33));
  this->blue_display  = (HT16K33*) malloc(sizeof(HT16K33));
  this->red_display   = (HT16K33*) malloc(sizeof(HT16K33));

  if (this->green_display == NULL ||
      this->blue_display == NULL ||
      this->red_display == NULL) {
    free(this);
    return NULL;
  }

  HT16K33_init(this->green_display, I2C_ADAPTER_1, GREEN_DISPLAY_ADDRESS);
  HT16K33_init(this->blue_display, I2C_ADAPTER_1, BLUE_DISPLAY_ADDRESS);
  HT16K33_init(this->red_display, I2C_ADAPTER_1, RED_DISPLAY_ADDRESS);
  
  // alias in array
  this->display_array[DISPLAY_GREEN] = this->green_display;
  this->display_array[DISPLAY_BLUE] = this->blue_display;
  this->display_array[DISPLAY_RED] = this->red_display;

  // error codes positive...just sum them up

  rc += initialize_backpack(this->green_display);
  rc += initialize_backpack(this->blue_display);
  rc += initialize_backpack(this->red_display);

  if (rc) {
    free(this);
    return NULL;
  }
  
  // error codes negative
  rc = HT16K33_COMMIT(this->green_display);
  rc -= HT16K33_COMMIT(this->blue_display);
  rc -= HT16K33_COMMIT(this->red_display);

  if (rc) {
    free(this);
    return NULL;
  }

  return this;
}


void display_view(struct game_view *this, struct game_model *model) {
  // odd..but abstract out implementation while passing object state.
  // maybe I should switch to an OO language?
  this->display(this, model);
}



int free_game_view(struct game_view *this) {
  int rc = 0;
  if (this == NULL) {
    return 1;
  }

  HT16K33_CLOSE(this->green_display);
  HT16K33_CLOSE(this->blue_display);
  HT16K33_CLOSE(this->red_display);

  free(this);
  return rc;

}


static void ht16k33_alphanum_display_game(struct game_view *this, struct game_model *model) {
  int backpack, digit;
  char *display[3];
  display[0] = get_green_string(model);
  display[1] = get_blue_string(model);
  display[2] = get_red_string(model);

  for (backpack = 0; backpack < DISPLAY_ROWS; ++backpack) { // backpack

    for (digit = 0; digit < 4; ++digit) { // digit
      if (display[backpack] == NULL ||
	  display[backpack][digit] == '\0') {
	break;
      }
      // no colon sep on alphanum packs
      HT16K33_UPDATE_ALPHANUM(this->display_array[backpack], digit, display[backpack][digit], 0);

    }

    for (; digit < 4; ++digit) {
      // clear any remaining digits
      HT16K33_CLEAN_DIGIT(this->display_array[backpack], digit);
    }

    HT16K33_COMMIT(this->display_array[backpack]);
  }

}




/*
void ht16k33_7_segment_display_game(game_model model) {
  int backpack, digit;

  for (backpack = 0; backpack < DISPLAY_ROWS; ++backpack) { // backpack
    HT16K33_CLEAN_DIGIT(model->display_array[backpack], 2); // colon off
    for (digit = 0; digit < 4; ++digit) {
      if (model->string_array[backpack] == NULL ||
	  model->string_array[backpack][digit] == '\0') {
	break;
      }
      // digit two is the colon sep, so skip that
      HT16K33_UPDATE_DIGIT(model->display_array[backpack], digit < 2 ? digit : digit + 1, model->string_array[backpack][digit], 0);

    }

    for (; digit < 5; ++digit) {
      // clear any remaining digits
      HT16K33_CLEAN_DIGIT(model->display_array[backpack], digit < 2 ? digit : digit + 1);
    }

    HT16K33_COMMIT(model->display_array[backpack]);
  }

}
*/






static int initialize_backpack(HT16K33 *backpack) {
  int rc;
  // prepare the backpack driver
  // (the first parameter is the raspberry pi i2c master controller attached to the HT16K33, the second is the i2c selection jumper)
  // The i2c selection address can be one of HT16K33_ADDR_01 to HT16K33_ADDR_08
  // initialize the backpack
  rc = HT16K33_OPEN(backpack);

  if(rc != 0) {
    fprintf(stderr, "Error initializing HT16K33 led backpack. Check your i2c bus (es. i2cdetect)\n");
    //fprintf(stderr, "Error initializing HT16K33 led backpack (%s). Check your i2c bus (es. i2cdetect)\n", strerror(backpack->lasterr));
    // you don't need to HT16K33_CLOSE() if HT16K33_OPEN() failed, but it's safe doing it.
    HT16K33_CLOSE(backpack);
    return rc;
  }

  rc = HT16K33_INTERRUPT(backpack, HT16K33_ROW15_DRIVER);
  if (rc != 0) {
    fprintf(stderr, "Error putting the HT16K33 led backpack to INTERRUPT_HIGH. Check your i2c bus (es. i2cdetect)\n");
    // you don't need to HT16K33_OFF() if HT16K33_ON() failed, but it's safe doing it.
    HT16K33_OFF(backpack);
    HT16K33_CLOSE(backpack);
    return rc;
  }

  // power on the ht16k33
  rc = HT16K33_ON(backpack);
  if(rc != 0) {
    fprintf(stderr, "Error putting the HT16K33 led backpack 1 ON (%s). Check your i2c bus (es. i2cdetect)\n", strerror(backpack->lasterr));
    // you don't need to HT16K33_OFF() if HT16K33_ON() failed, but it's safe doing it.
    HT16K33_OFF(backpack);
    HT16K33_CLOSE(backpack);
    return rc;
  }

  // halfway bright
  HT16K33_BRIGHTNESS(backpack, 0x07);

  // power on the display
  HT16K33_DISPLAY(backpack, HT16K33_DISPLAY_ON);

  return rc;
}
