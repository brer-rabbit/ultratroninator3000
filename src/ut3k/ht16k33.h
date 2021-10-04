/**
 * Copyright 2021 Kyle Farrell <ultratroninator3000@patentinvestor.com>
 * Based upon work from Dino Ciuffetti
 *
 * Copyright 2014 Dino Ciuffetti <dino@tuxweb.it>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HT16K33_H
#define HT16K33_H

#include <stdint.h>
#include <linux/i2c-dev.h>

/**
 * Adafruit 7 segment backpack is shipped with a 28-Pin package HT16K33.
 * You can have up to 8 devices running on the same bus. There are 3 jumpers to select the i2c slave address.
 * 
 * MSB           LSB
 * 1 1 1 0 A2 A1 A0
 */
enum
{
	HT16K33_ADDR_01 = 0x70,	// i2c slave selection jumpers: (A2: 0, A1: 0, A0: 0)
	HT16K33_ADDR_02 = 0x71, // i2c slave selection jumpers: (A2: 0, A1: 0, A0: 1)
	HT16K33_ADDR_03 = 0x72, // i2c slave selection jumpers: (A2: 0, A1: 1, A0: 0)
	HT16K33_ADDR_04 = 0x73, // i2c slave selection jumpers: (A2: 0, A1: 1, A0: 1)
	HT16K33_ADDR_05 = 0x74, // i2c slave selection jumpers: (A2: 1, A1: 0, A0: 0)
	HT16K33_ADDR_06 = 0x75, // i2c slave selection jumpers: (A2: 1, A1: 0, A0: 1)
	HT16K33_ADDR_07 = 0x76, // i2c slave selection jumpers: (A2: 1, A1: 1, A0: 0)
	HT16K33_ADDR_08 = 0x77  // i2c slave selection jumpers: (A2: 1, A1: 1, A0: 1)
};

typedef enum
{
	HT16K33_BLINK_OFF = 0x00,	// disable display blinking
	HT16K33_BLINK_FAST = 0x02,	// display blinks at 2 HZ
	HT16K33_BLINK_NORMAL = 0x04,	// display blinks at 1 HZ
	HT16K33_BLINK_SLOW = 0x06	// display blinks at 0.5 HZ
} ht16k33blink_t;


typedef enum
{
        HT16K33_BRIGHTNESS_0 = 0x00,  // lowest setting, 1/16 duty
        HT16K33_BRIGHTNESS_1 = 0x01,
        HT16K33_BRIGHTNESS_2 = 0x02,
        HT16K33_BRIGHTNESS_3 = 0x03,
        HT16K33_BRIGHTNESS_4 = 0x04,
        HT16K33_BRIGHTNESS_5 = 0x05,
        HT16K33_BRIGHTNESS_6 = 0x06,
        HT16K33_BRIGHTNESS_7 = 0x07,
        HT16K33_BRIGHTNESS_8 = 0x08,
        HT16K33_BRIGHTNESS_9 = 0x09,
        HT16K33_BRIGHTNESS_10 = 0x0A,
        HT16K33_BRIGHTNESS_11 = 0x0B,
        HT16K33_BRIGHTNESS_12 = 0x0C,
        HT16K33_BRIGHTNESS_13 = 0x0D,
        HT16K33_BRIGHTNESS_14 = 0x0E,
        HT16K33_BRIGHTNESS_15 = 0x0F  // highest setting, 16/16 duty
} ht16k33brightness_t;

typedef enum
{
	HT16K33_DISPLAY_OFF = 0x00,	// disable display (all leds off)
	HT16K33_DISPLAY_ON = 0x01,	// enable display
} ht16k33display_t;

// - wake up HT16K33 by setting the S option bit of the "system setup register"
// - set HT16K33 to sleep by clearing the S option bit of the "system setup register"
// "system setup register" starts at 0x20h.
// DEVICE ON: 0x21 (00100001), DEVICE OFF: 0x20 (00100000)
#define HT16K33_WAKEUP_OP 0x21
#define HT16K33_SLEEP_OP 0x20

// display setup register base address
#define HT16K33_DISPLAY_SETUP_BASE 0x80

// digital dimming (display brightness) command base address
#define HT16K33_DIMMING_BASE 0xE0

#define HT16K33_ZERO 127
#define HT16K33_FULL 128

// Interrupt set command
#define HT16K33_INTERRUPT_BASE 0xA0
typedef enum
{
    HT16K33_ROW15_DRIVER = 0x00,   // row 15 is a row driver
    HT16K33_ROW15_INTERRUPT_LOW = 0x01,   // row 15 is active low interrupt
    HT16K33_ROW15_INTERRUPT_HIGH = 0x03   // row 15 is active high interrupt
} ht16k33interrupt_t;


typedef uint8_t ht16k33keyscan_t[6];

#define ht16k33keyscan_byte(keyscan_buffer, byte_num) ((*(keyscan_buffer))[(byte_num)])

#define HT16K33_KEY_DATA_RAM_BASE 0x40





typedef struct ht16k33_matrix
{
	/*
	 * In Adafruit 7seg display backpack, each com is mapped to a display digit:
	 * COM0 COM1 COM3 COM4: 1/2/3/4 digit; COM2: colon
	 * Adafruit Alphanum uses four com lines
	 * COM0 COM1 COM2 COM3
	 * Ultratroninator uses 3 com lines:
	 * COM4 (red) COM5 (blue) COM6 (green)
	 *
	 * store each COM line as two consecutive bytes
	*/
	uint8_t com[16];
} ht16k33_matrix;

typedef struct HT16K33
{
	int adapter_nr;				// i2c adapter number (0 => /dev/i2c-0 | 1 => /dev/i2c-1)
	int adapter_fd;				// i2c file descriptor (opened for ex. from /dev/i2c-1)
	uint8_t driver_addr;			// i2c device address. Ex: HT16K33_ADDR_01
	int lasterr;				// last error number
	ht16k33interrupt_t interrupt_mode;	// interrupt mode
	ht16k33display_t display_state;		// backpack display state
	ht16k33blink_t blink_state;		// backpack blink state
	ht16k33brightness_t brightness;         // only the last nibble is used
	ht16k33_matrix display_buffer;		// adafruit 7 segment backpack display
} HT16K33;

/* INITIALIZATION FUNCTIONS AND MACRO */
/**
 * Prepare the driver.
 * The first parameter is the raspberry pi i2c master controller attached to the HT16K33, the second is the i2c selection jumper.
 * The i2c selection address can be one of HT16K33_ADDR_01 to HT16K33_ADDR_08
 */
#define HT16K33_INIT(i2cadapter, driveraddr) { \
	.adapter_nr = i2cadapter, \
	.adapter_fd = -1, \
	.driver_addr = driveraddr, \
	.lasterr = 0, \
        .interrupt_mode = HT16K33_ROW15_DRIVER, \
	.display_state = HT16K33_DISPLAY_OFF, \
	.blink_state = HT16K33_BLINK_OFF, \
	.brightness = HT16K33_BRIGHTNESS_15, /* 16/16 duty (max brightness) */ \
      .display_buffer = { { 0, 0, 0, 0, 0, 0, 0, 0 } },	      \
};


void HT16K33_init(HT16K33 *backpack, int i2cadapter, uint8_t driveraddr);


/**
 * Initialize the driver
 */
int HT16K33_OPEN(HT16K33 *backpack);
/**
 * Close the driver
 */
void HT16K33_CLOSE(HT16K33 *backpack);
/**
 * Wake up the HT16K33.
 */
int HT16K33_ON(HT16K33 *backpack);
/**
 * Put to sleep the HT16K33.
 */
int HT16K33_OFF(HT16K33 *backpack);
/**
 * set interrupt mode or turn off for row15
 */
int HT16K33_INTERRUPT(HT16K33 *backpack, ht16k33interrupt_t interrupt_mode);
/**
 * Make the display blinking (or not).
 * The blink speed can be setted with the blink_cmd attribute that can be one of:
 * HT16K33_BLINK_OFF (no blink), HT16K33_BLINK_SLOW (0.5 HZ), HT16K33_BLINK_NORMAL (1 HZ), HT16K33_BLINK_FAST (2 HZ)
 */
int HT16K33_BLINK(HT16K33 *backpack, ht16k33blink_t blink_cmd);
/**
 * Enable or disable the display.
 * Disabling the display will also put it in power saving mode.
 * The attribute display_cmd can be one of HT16K33_DISPLAY_OFF (to disable the display) or HT16K33_DISPLAY_ON (to enable the display)
 */
int HT16K33_DISPLAY(HT16K33 *backpack, ht16k33display_t display_cmd);
/**
 * Set the display brightness.
 * The attribute brightness can be setted from 0x00 (minimum brightness) to 0x0F (maximum brightness).
 * When brightness is setted to 0x00 the dimming duty is 1/16, when setted to 0x0F the dimming duty is 16/16.
 */
int HT16K33_BRIGHTNESS(HT16K33 *backpack, ht16k33brightness_t brightness);
/**
 * Adafruit 7-segment display:
 * Update a single display digit, identified by the digit parameter, starting from 0.
 * The digit 2 is the colon sign in the middle of the four digits.
 * If you want to enable the decimal point on the given digit, pass 1 to the "decimal_point" attribute
 * To make it reflected on the 7 segment display you have to call HT16K33_COMMIT()
 */
int HT16K33_UPDATE_DIGIT(HT16K33 *backpack, unsigned short digit, const unsigned char value, unsigned short decimal_point);

/** 
 * Adafruit alphanumt display:
 * Update a single display digit, identified by the digit parameter, starting from 0.
 * If you want to enable the decimal point on the given digit, pass 1 to the "decimal_point" attribute
 * To make it reflected on the display you have to call HT16K33_COMMIT()
 */
int HT16K33_UPDATE_ALPHANUM(HT16K33 *backpack, unsigned short digit, const unsigned char value, unsigned short decimal_point);
// as above, but just raw data / no ascii mapping and takes all 16 bits
// To make it reflected on the display you have to call HT16K33_COMMIT()
// value is assumed to be an array of four...
int HT16K33_UPDATE_RAW(HT16K33 *backpack, const uint16_t value[]);
int HT16K33_UPDATE_RAW_BYDIGIT(HT16K33 *backpack, unsigned short digit, const uint16_t value);

/** write all the digits of an integer to the display
 * call HT16K33_COMMIT to display
 */
int HT16K33_DISPLAY_INTEGER(HT16K33 *backpack, int16_t value);

/**
 * Clean a single display digit, identified by the digit parameter, starting from 0.
 * The digit 2 is the colon sign in the middle of the four digits.
 * To make it reflected on the 7 segment display you have to call HT16K33_COMMIT()
 */
int HT16K33_CLEAN_DIGIT(HT16K33 *backpack, unsigned short digit);
/**
 * Commit the display buffer data to the 7 segment display, showing the saved data.
 * Data must be saved to the buffer calling HT16K33_UPDATE_DIGIT() before calling this one.
 */
int HT16K33_COMMIT(HT16K33 *backpack);


/**
 * Read they key scanner memeory from the HT16K33
 */
int HT16K33_READ(HT16K33 *backpack, ht16k33keyscan_t keyscan);


/** Some example of macros writing things on the display **/
/**
 * Say "Hello"
 */
#define HT16K33_SAY_HELLO(backpack) \
	HT16K33_UPDATE_DIGIT(backpack, 0, 'H', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 1, 'E', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 2, HT16K33_COLON_OFF, 0); \
	HT16K33_UPDATE_DIGIT(backpack, 3, '#', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 4, 'o', 0); \
	HT16K33_COMMIT(backpack);
/**
 * Say "no"
 */
#define HT16K33_SAY_NO(backpack) \
	HT16K33_UPDATE_DIGIT(backpack, 0, HT16K33_ZERO, 0); \
	HT16K33_UPDATE_DIGIT(backpack, 1, 'n', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 2, HT16K33_COLON_OFF, 0); \
	HT16K33_UPDATE_DIGIT(backpack, 3, 'o', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 4, HT16K33_ZERO, 0); \
	HT16K33_COMMIT(backpack);
/**
 * Say "yES"
 */
#define HT16K33_SAY_YES(backpack) \
	HT16K33_UPDATE_DIGIT(backpack, 0, 'y', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 1, 'E', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 2, HT16K33_COLON_OFF, 0); \
	HT16K33_UPDATE_DIGIT(backpack, 3, 'S', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 4, HT16K33_ZERO, 0); \
	HT16K33_COMMIT(backpack);
/**
 * Say "PLAy"
 */
#define HT16K33_SAY_PLAY(backpack) \
	HT16K33_UPDATE_DIGIT(backpack, 0, 'P', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 1, 'L', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 2, HT16K33_COLON_OFF, 0); \
	HT16K33_UPDATE_DIGIT(backpack, 3, 'A', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 4, 'y', 0); \
	HT16K33_COMMIT(backpack);
/**
 * Say "HELP"
 */
#define HT16K33_SAY_HELP(backpack) \
	HT16K33_UPDATE_DIGIT(backpack, 0, 'H', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 1, 'E', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 2, HT16K33_COLON_OFF, 0); \
	HT16K33_UPDATE_DIGIT(backpack, 3, 'L', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 4, 'P', 0); \
	HT16K33_COMMIT(backpack);
/**
 * Say "you"
 */
#define HT16K33_SAY_YOU(backpack) \
	HT16K33_UPDATE_DIGIT(backpack, 0, 'y', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 1, 'o', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 2, HT16K33_COLON_OFF, 0); \
	HT16K33_UPDATE_DIGIT(backpack, 3, 'u', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 4, HT16K33_ZERO, 0); \
	HT16K33_COMMIT(backpack);
/**
 * Say "COOL"
 */
#define HT16K33_SAY_COOL(backpack) \
	HT16K33_UPDATE_DIGIT(backpack, 0, 'C', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 1, 'O', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 2, HT16K33_COLON_OFF, 0); \
	HT16K33_UPDATE_DIGIT(backpack, 3, 'O', 0); \
	HT16K33_UPDATE_DIGIT(backpack, 4, 'L', 0); \
	HT16K33_COMMIT(backpack);

#endif
