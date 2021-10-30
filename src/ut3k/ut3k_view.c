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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/gpio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include "ut3k_view.h"

#define GREEN_DISPLAY_ADDRESS HT16K33_ADDR_07
#define BLUE_DISPLAY_ADDRESS HT16K33_ADDR_06
#define RED_DISPLAY_ADDRESS HT16K33_ADDR_05
#define INPUTS_AND_LEDS_ADDRESS HT16K33_ADDR_04
#define I2C_ADAPTER_1 1

#define DISPLAY_GREEN 0
#define DISPLAY_BLUE 1
#define DISPLAY_RED 2
#define DISPLAY_ROWS 3


static const int gpio_rotary_green_a_bcm = 16;
static const int gpio_rotary_green_b_bcm = 26;
static const int gpio_rotary_blue_a_bcm = 6;
static const int gpio_rotary_blue_b_bcm = 13;
static const int gpio_rotary_red_a_bcm = 12;
static const int gpio_rotary_red_b_bcm = 25;
static const char *gpio_devfile = "/dev/gpiochip0";



// I've no idea why this is parameterized here like this...why did I do that?
typedef void (*f_show_displays)(struct ut3k_view*, struct display_strategy*);

// implements f_show_displays
static void ht16k33_alphanum_display_game(struct ut3k_view *this, struct display_strategy *display);

/** rotary encoder stuff
 *
 * runs in own thread, reading the rotary encoders at a faster
 * interval than the HT16K33 can provide.
 * Use a long long int (64 bits) as a queue for each encoder.
 * Each read push two bits onto the end.
 */
struct rotary_encoder_bits_queue {
  unsigned long long int bit_queue;
  int queue_index;
  int previous_a;
  int previous_b;
};
static void* poll_rotary_encoders(void *userdata);


struct ut3k_view {

  // Adafruit displays with backpacks
  HT16K33 *green_display;
  HT16K33 *blue_display;
  HT16K33 *red_display;

  // aliases for the above
  HT16K33 *display_array[3];

  // bunch-o-knobs and the 3 sets of discrete LEDs are here
  HT16K33 *inputs_and_leds;

  struct control_panel *control_panel;
  void *control_panel_listener_userdata;  // for callback
  f_view_control_panel_listener control_panel_listener;  // the callback

  // rotary encoder baggage
  pthread_t thread_poll_rotary_encoders;
  pthread_mutex_t rotary_bits_queue_mutex;
  // the data below should only be accessed by the rotary_bits_queue_mutex
  struct rotary_encoder_bits_queue green_rotary_queue;
  struct rotary_encoder_bits_queue blue_rotary_queue;
  struct rotary_encoder_bits_queue red_rotary_queue;
// end mutex protected data
  int cleanup_and_exit; // signal to thread to exit
};


/** initialize_backpack
 *
 * initialize a single HT16K33 chip
 */
static int initialize_backpack(HT16K33 *backpack);



/** create_alphanum_ut3k_view
 * clients will want to call this function with a ut3k_view to construct
 * an alphanum view.
 * Creates a view that utilizes three of the Adafruit HT16K33 alphanum
 * displays
 */
struct ut3k_view* create_alphanum_ut3k_view() {
  struct ut3k_view *this;
  int rc = 0;
  ht16k33keyscan_t keyscan;


  this = (struct ut3k_view*) malloc(sizeof(struct ut3k_view));

  this->green_display = (HT16K33*) malloc(sizeof(HT16K33));
  this->blue_display  = (HT16K33*) malloc(sizeof(HT16K33));
  this->red_display   = (HT16K33*) malloc(sizeof(HT16K33));
  this->inputs_and_leds = (HT16K33*) malloc(sizeof(HT16K33));

  if (this->green_display == NULL ||
      this->blue_display == NULL ||
      this->red_display == NULL ||
      this->inputs_and_leds == NULL) {
    free(this);
    return NULL;
  }

  HT16K33_init(this->green_display, I2C_ADAPTER_1, GREEN_DISPLAY_ADDRESS);
  HT16K33_init(this->blue_display, I2C_ADAPTER_1, BLUE_DISPLAY_ADDRESS);
  HT16K33_init(this->red_display, I2C_ADAPTER_1, RED_DISPLAY_ADDRESS);
  HT16K33_init(this->inputs_and_leds, I2C_ADAPTER_1, INPUTS_AND_LEDS_ADDRESS);
  
  // alias in array
  this->display_array[DISPLAY_GREEN] = this->green_display;
  this->display_array[DISPLAY_BLUE] = this->blue_display;
  this->display_array[DISPLAY_RED] = this->red_display;

  // error codes positive...just sum them up

  rc += initialize_backpack(this->green_display);
  rc += initialize_backpack(this->blue_display);
  rc += initialize_backpack(this->red_display);
  rc += initialize_backpack(this->inputs_and_leds);

  if (rc) {
    free(this);
    return NULL;
  }
  
  // error codes negative
  rc = HT16K33_COMMIT(this->green_display);
  rc -= HT16K33_COMMIT(this->blue_display);
  rc -= HT16K33_COMMIT(this->red_display);
  rc -= HT16K33_COMMIT(this->inputs_and_leds);

  if (rc) {
    free(this);
    return NULL;
  }



  // get current state of control panel.  Some switches may
  // be set, so start off with the state reflecting that.
  rc = HT16K33_READ(this->inputs_and_leds, keyscan);
  if (rc != 0) {
    printf("keyscan failed with code %d\n", rc);
  }

  this->control_panel = create_control_panel(keyscan);

  // init and start polling the rotary encoders
  this->green_rotary_queue = (struct rotary_encoder_bits_queue const) { .bit_queue = 0, .queue_index = 0, .previous_a = 1, .previous_b = 1 };
  this->blue_rotary_queue = (struct rotary_encoder_bits_queue const) { .bit_queue = 0, .queue_index = 0, .previous_a = 1, .previous_b = 1 };
  this->red_rotary_queue = (struct rotary_encoder_bits_queue const) { .bit_queue = 0, .queue_index = 0, .previous_a = 1, .previous_b = 1 };
  pthread_mutex_init(&this->rotary_bits_queue_mutex, NULL);
  this->cleanup_and_exit = 0;

  rc = pthread_create(&this->thread_poll_rotary_encoders, NULL, poll_rotary_encoders, this);
  if (rc != 0) {
    printf("create_alphanum_ut3k_view: failed to start rotary encoder listener %d\n", rc);
  }

  return this;
}




int free_ut3k_view(struct ut3k_view *this) {
  int rc = 0;
  if (this == NULL) {
    return 1;
  }

  // signal thread to exit
  this->cleanup_and_exit = 1;

  free_control_panel(this->control_panel);

  HT16K33_CLOSE(this->green_display);
  HT16K33_CLOSE(this->blue_display);
  HT16K33_CLOSE(this->red_display);
  HT16K33_CLOSE(this->inputs_and_leds);

  free(this->green_display);
  free(this->blue_display);
  free(this->red_display);
  free(this->inputs_and_leds);

  pthread_join(this->thread_poll_rotary_encoders, NULL);

  free(this);

  return rc;

}


/** update_view
 * keyscan HT16K33
 * callback to control panel listener (if non-null/registered)
 */
void update_controls(struct ut3k_view *this, uint32_t clock) {
  ht16k33keyscan_t keyscan;
  int keyscan_rc;
  unsigned long long int green_bit_queue, blue_bit_queue, red_bit_queue;
  int green_queue_index, blue_queue_index, red_queue_index;

  // only read every other clock cycle
  if (clock & 0b1) {
    keyscan_rc = HT16K33_READ(this->inputs_and_leds, keyscan);
    if (keyscan_rc != 0) {
      printf("keyscan failed with code %d\n", keyscan_rc);
    }

    //  printf("keyscan: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
    //  	 keyscan[0], keyscan[1], keyscan[2], keyscan[3], keyscan[4], keyscan[5]);

    // drain the queue: read any rotary encoder data
    // treat this as a critical section- just copy the data
    // out and release the lock.  Minimize losing any reads
    // from the gpio pins.  Maybe I'm just paranoid.
    pthread_mutex_lock(&this->rotary_bits_queue_mutex);
    if (this->green_rotary_queue.queue_index > 3) {
      green_bit_queue = this->green_rotary_queue.bit_queue;
      green_queue_index = this->green_rotary_queue.queue_index;
      // leave the last two
      this->green_rotary_queue.bit_queue = this->green_rotary_queue.bit_queue & 0b11;
      this->green_rotary_queue.queue_index = 2;
    }
    else {  // no change-- queue size zero for no processing
      green_bit_queue = 0;
      green_queue_index = 0;
    }

    if (this->blue_rotary_queue.queue_index > 3) {
      blue_bit_queue = this->blue_rotary_queue.bit_queue;
      blue_queue_index = this->blue_rotary_queue.queue_index;
      // leave the last two
      this->blue_rotary_queue.bit_queue = this->blue_rotary_queue.bit_queue & 0b11;
      this->blue_rotary_queue.queue_index = 2;
    }
    else {
      blue_bit_queue = 0;
      blue_queue_index = 0;
    }

    if (this->red_rotary_queue.queue_index > 3) {
      red_bit_queue = this->red_rotary_queue.bit_queue;
      red_queue_index = this->red_rotary_queue.queue_index;
      // leave the last two
      this->red_rotary_queue.bit_queue = this->red_rotary_queue.bit_queue & 0b11;
      this->red_rotary_queue.queue_index = 2;
    }
    else {
      red_bit_queue = 0;
      red_queue_index = 0;
    }

    pthread_mutex_unlock(&this->rotary_bits_queue_mutex);

    // update control panel here...
    update_control_panel(this->control_panel, keyscan,
			 green_bit_queue, green_queue_index,
			 blue_bit_queue, blue_queue_index,
			 red_bit_queue, red_queue_index,
			 clock);

    if (this->control_panel_listener) {
      (*this->control_panel_listener)((const struct control_panel*)this->control_panel, this->control_panel_listener_userdata);
    }
  }


}




/** update_displays
 * keyscan HT16K33
 * callback to any hooks
 */
void update_displays(struct ut3k_view *this, struct display_strategy *display_strategy, uint32_t clock) {
  // odd..but abstract out implementation while passing object state.
  // maybe I should switch to an OO language?
  // show_displays is expected to update all visual info on the HT16K33s
  ht16k33_alphanum_display_game(this, display_strategy);

}




/* Listener ---------------------------------------------------------- */



void register_control_panel_listener(struct ut3k_view *view, f_view_control_panel_listener f, void *userdata) {
  view->control_panel_listener = f;
  view->control_panel_listener_userdata = userdata;
}


/* Accessors ---------------------------------------------------------- */

const struct control_panel* get_control_panel(struct ut3k_view *this) {
  return (const struct control_panel*) this->control_panel;
}



/* Static ------------------------------------------------------------- */


/** commit_string
 * 
 * write a string to a specific HT16K33 display.  Only the first 4
 * chars are written.  Commit is called.
 */
static void commit_string(HT16K33 *display, char *string) {
  int digit;

  for (digit = 0; digit < 4; ++digit) { // digit
    if (string == NULL || string[digit] == '\0') {
      break;
    }
    HT16K33_UPDATE_ALPHANUM(display, digit, string[digit], 0);
  }

  for (; digit < 4; ++digit) {
    // clear any remaining digits
    HT16K33_CLEAN_DIGIT(display, digit);
  }

  HT16K33_COMMIT(display);
}



/** commit_glyph
 *
 * raw read of four 16 bit ints sent straight to the update raw method.
 * more than enough rope to hang yourself with this.
 */
static void commit_glyph(HT16K33 *display, uint16_t glyph[]) {
  HT16K33_UPDATE_RAW(display, glyph);
  HT16K33_COMMIT(display);
}



/** commit_integer
 * 
 * write a string to a specific HT16K33 display.  Only the first 4
 * chars are written.  Commit is called.
 */
static void commit_integer(HT16K33 *display, int16_t value) {

  if (value >= 0 && value < 256) {
    HT16K33_DISPLAY_INTEGER(display, (uint8_t)value);
  }
  else {
    char buffer[5];
    snprintf(buffer, 5, "%4hd", value);

    for (int digit = 0; digit < 4; ++digit) { // digit
      HT16K33_UPDATE_ALPHANUM(display, digit, buffer[digit], 0);
    }
  }

  HT16K33_COMMIT(display);
}





/** ht16k33_alphanum_display_game: implements f_show_displays
 * specific implementation for the Adafruit alphanum display
 */

static void ht16k33_alphanum_display_game(struct ut3k_view *this, struct display_strategy *display_strategy) {
  display_value union_result;
  ht16k33blink_t blink;
  ht16k33brightness_t brightness;
  uint32_t led_display_value = 0;
  

  switch(display_strategy->get_green_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
    commit_integer(this->display_array[0], union_result.display_int);
    break;
  case glyph_display:
    commit_glyph(this->display_array[0], union_result.display_glyph);
    break;
  case string_display:
    commit_string(this->display_array[0], union_result.display_string);
    break;
  }

  if (brightness != this->green_display->brightness) {
    HT16K33_BRIGHTNESS(this->green_display, brightness);
  }
  if (blink != this->green_display->blink_state) {
    HT16K33_BLINK(this->green_display, blink);
  }



  switch(display_strategy->get_blue_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
    commit_integer(this->display_array[1], union_result.display_int);
    break;
  case glyph_display:
    commit_glyph(this->display_array[1], union_result.display_glyph);
    break;
  case string_display:
    commit_string(this->display_array[1], union_result.display_string);
    break;
  }

  if (brightness != this->blue_display->brightness) {
    HT16K33_BRIGHTNESS(this->blue_display, brightness);
  }
  if (blink != this->blue_display->blink_state) {
    HT16K33_BLINK(this->blue_display, blink);
  }



  switch(display_strategy->get_red_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
    commit_integer(this->display_array[2], union_result.display_int);
    break;
  case glyph_display:
    commit_glyph(this->display_array[2], union_result.display_glyph);
    break;
  case string_display:
    commit_string(this->display_array[2], union_result.display_string);
    break;
  }


  if (brightness != this->red_display->brightness) {
    HT16K33_BRIGHTNESS(this->red_display, brightness);
  }
  if (blink != this->red_display->blink_state) {
    HT16K33_BLINK(this->red_display, blink);
  }



  switch(display_strategy->get_leds_display(display_strategy, &union_result, &blink, &brightness)) {
  case integer_display:
    led_display_value = union_result.display_int;
    break;
  case glyph_display:
  case string_display:
    printf("only integer supported\n");
    return;
  }

  if (brightness != this->inputs_and_leds->brightness) {
    HT16K33_BRIGHTNESS(this->inputs_and_leds, brightness);
  }
  if (blink != this->inputs_and_leds->blink_state) {
    HT16K33_BLINK(this->inputs_and_leds, blink);
  }

  HT16K33_UPDATE_RAW_BYDIGIT(this->inputs_and_leds, 4, (uint16_t)(led_display_value & 0x00FF));
  HT16K33_UPDATE_RAW_BYDIGIT(this->inputs_and_leds, 5, (uint16_t)((led_display_value >> 8) & 0x00FF));
  HT16K33_UPDATE_RAW_BYDIGIT(this->inputs_and_leds, 6, (uint16_t)((led_display_value >> 16) & 0x00FF));
  HT16K33_COMMIT(this->inputs_and_leds);

}





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

  rc = HT16K33_INTERRUPT(backpack, HT16K33_ROW15_INTERRUPT_HIGH);
  if (rc != 0) {
    fprintf(stderr, "Error putting the HT16K33 led backpack to ROW15 Driver. Check your i2c bus (es. i2cdetect)\n");
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
  HT16K33_BRIGHTNESS(backpack, HT16K33_BRIGHTNESS_7);

  // no blink
  HT16K33_BLINK(backpack, HT16K33_BLINK_OFF);

  // power on the display
  HT16K33_DISPLAY(backpack, HT16K33_DISPLAY_ON);

  return rc;
}


/** push encoder queue
 *
 * maybe push might be a better name: push both a &b if one of a || b values
 * is different from what was previously seen.
 * The queue is a 64 bit int.
 * "front" of the queue has the most recent two bits read, with older
 * pairs of bits following:
 *
 *                   queue_index = 6                  
 * ------------- ... ---------------------------------------
 * |    |    |   ...    |    | B2 | A2 | B1 | A1 | B0 | A0 |
 * ------------- ... ---------------------------------------
 *   63   62              6    5    4    3    2    1    0
 *
 * queue_index = 0 :: nothing in the queue
 * queue_index = 1 :: only one state in the queue
 * queue_index > 1 :: two ore more states in queue: these can be processed
 *                     via lookup table
 */
static inline void push_encoder_queue(struct rotary_encoder_bits_queue *queue, int a, int b) {

  assert(queue->queue_index < (sizeof(long long int) * 8));
  if (queue->previous_a != a || queue->previous_b != b) {
    queue->bit_queue = (queue->bit_queue << 2) | (a) | (b << 1);
    queue->queue_index += 2;
    queue->previous_a = a;
    queue->previous_b = b;
  }
  
}


static void* poll_rotary_encoders(void *userdata) {
  struct ut3k_view *this = (struct ut3k_view*) userdata;
  int gpio_fd = -1; // set to an invalid fd
  int ret;
  struct gpiohandle_request gpio_request;
  struct gpiohandle_data gpio_data;

  
  gpio_fd = open(gpio_devfile, O_RDONLY);
  if (gpio_fd == -1) {
    printf("can't open gpio devfile, %s\n", strerror(errno));
    return NULL;
  }

  // setup to read 2 bits from 3 encoders.  Pullup resistors required.
  gpio_request.lineoffsets[0] = gpio_rotary_green_a_bcm;
  gpio_request.lineoffsets[1] = gpio_rotary_green_b_bcm;
  gpio_request.lineoffsets[2] = gpio_rotary_blue_a_bcm;
  gpio_request.lineoffsets[3] = gpio_rotary_blue_b_bcm;
  gpio_request.lineoffsets[4] = gpio_rotary_red_a_bcm;
  gpio_request.lineoffsets[5] = gpio_rotary_red_b_bcm;
  gpio_request.lines = 6;
  gpio_request.flags = GPIOHANDLE_REQUEST_INPUT | GPIOHANDLE_REQUEST_BIAS_PULL_UP;
  strcpy(gpio_request.consumer_label, "ut3k_view");
  
  if (ioctl(gpio_fd, GPIO_GET_LINEHANDLE_IOCTL, &gpio_request) == -1) {
    printf("unable to get line handle via ioctl: %s (%d)\n", strerror(errno), errno);
    return NULL;
  }

  // can now close the devfile handle
  ret = close(gpio_fd);
  assert(ret == 0);

  printf("starting thread rotary encoder loop\n");

  // start read loop for encoder pins
  do {
    ret = ioctl(gpio_request.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &gpio_data);
    if (ret == -1) {
	printf("Unable to get line value using ioctl : %s\n", strerror(errno));
    }
    else {
      ret = pthread_mutex_lock(&this->rotary_bits_queue_mutex);

      // mutex locked, have access to the encoder queues
      push_encoder_queue(&this->green_rotary_queue, gpio_data.values[0], gpio_data.values[1]);
      push_encoder_queue(&this->blue_rotary_queue, gpio_data.values[2], gpio_data.values[3]);
      push_encoder_queue(&this->red_rotary_queue, gpio_data.values[4], gpio_data.values[5]);

      pthread_mutex_unlock(&this->rotary_bits_queue_mutex);
    }

    usleep(2500);

  } while (this->cleanup_and_exit == 0);


  close(gpio_request.fd);

  return NULL;
}
