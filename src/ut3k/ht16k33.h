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
#define ht16k33keyscan_byte(keyscan_buffer, byte_num) ((*keyscan_buffer)[byte_num])

#define HT16K33_KEY_DATA_RAM_BASE 0x40


/**
  Adafruit 7 segment display

  DP is the decimal point at the right of each 7 segment digit
  CL is a colon sign (:) between the 2 couple of seven segment digits (ex. 01:02)

      A                    ROW0 - A    - 00000001(0X01)
     ___            CL     ROW1 - B,CL - 00000010(0X02)
    |   |           .      ROW2 - C    - 00000100(0X04)
  F | G | B                ROW3 - D    - 00001000(0X08)
    |---|                  ROW4 - E    - 00010000(0X10)
  E |   | C         .      ROW5 - F    - 00100000(0X20)
    |___|   . DP           ROW6 - G    - 01000000(0X40)
                           ROW7 - DP   - 10000000(0X80)
      D
                                         dGFEDCBA (d is alias for DP: decimal point)

                           COM0 - selects the first display digit
                           COM1 - selects the second display digit
                           COM2 - selects the colon sign between the two couple of digits
                           COM3 - selects the third display digit
                           COM4 - selects the fourth display digit

0: ABCDEF: ROWS(0,1,2,3,4,5) (0x3F)   8: ABCDEFG: ROWS(0,1,2,3,4,5,6) (0x7F)
1: BC: ROWS(1,2)             (0x06)   9: ABCDFG: ROWS(0,1,2,3,5,6)    (0x6F)
2: ABDEG: ROWS(0,1,3,4,6)    (0x5B)   A: ABCEFG: ROWS(0,1,2,4,5,6)    (0x77)
3: ABCDG: ROWS(0,1,2,3,6)    (0x4F)   b: CDEFG: ROWS(2,3,4,5,6)       (0x7C)
4: BCFG: ROWS(1,2,5,6)       (0x66)   C: ADEF: ROWS(0,3,4,5)          (0x39)
5: ACDFG: ROWS(0,2,3,5,6)    (0x6D)   d: BCDEG: ROWS(1,2,3,4,6)       (0x5E)
6: ACDEFG: ROWS(0,2,3,4,5,6) (0x7D)   E: ADEFG: ROWS(0,3,4,5,6)       (0x79)
7: ABC: ROWS(0,1,2)          (0x07)   F: AEFG: ROWS(0,4,5,6)          (0x71)

c: DEG    (0x58)   L: DEF    (0x38)   y: BCDFG (0x6E)
h: CEFG   (0x74)   o: CDEG   (0x5C)   ... check the table below ...
H: BCEFG  (0x76)   P: ABEFG  (0x73)
i: E      (0x10)   T: DEFG   (0x78)
I: EF     (0x30)   u: CDE    (0x1C)
J: BCDE   (0x1E)   U: BCDEF  (0x3E)

*/
static const uint8_t ht16k33_7seg_digits[] = {
	/* 0 */ 0x3F, /* 1 */ 0x06, /* 2 */ 0x5B, /* 3 */ 0x4F,
	/* 4 */ 0x66, /* 5 */ 0x6D, /* 6 */ 0x7D, /* 7 */ 0x07,
	/* 8 */ 0x7F, /* 9 */ 0x6F, /* A */ 0x77, /* b */ 0x7C,
	/* C */ 0x39, /* d */ 0x5E, /* E */ 0x79, /* F */ 0x71,
	
	/* PADDING to reach the space char " " (ascii dec 32) */
	/* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89,
	/* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89, /* [n/a] */ 0x89,
	/* [n/a] */ 0x89, /* [n/a] */ 0x89,
	
	/* n/a: not available; cnf: graphically equal to another char */
	/* (32) " " */ 0x00, /* (33) "!" */ 0x42, /* (34) """ */ 0x22, /* (35) "#" */ 0x36, 
	/* (36) "$" [n/a] */ 0x89, /* (37) "%" */ 0xD2, /* (38) "&" [n/a] */ 0x89, /* (39) "'" */ 0x02, /* (40) "(" [cnf] */ 0x39, 
	/* (41) ")" */ 0x0F, /* (42) "*" */ 0x76, /* (43) "+" [n/a] */ 0x89, /* (44) "," [cnf] */ 0x80, /* (45) "-" */ 0x40, 
	/* (46) "." [cnf] */ 0x80, /* (47) "/" */ 0x52, /* (48) "0" */ 0x3F, /* (49) "1" */ 0x06, /* (50) "2" */ 0x5B, 
	/* (51) "3" */ 0x4F, /* (52) "4" */ 0x66, /* (53) "5" */ 0x6D, /* (54) "6" */ 0x7D, /* (55) "7" */ 0x07, 
	/* (56) "8" */ 0x7F, /* (57) "9" */ 0x6F, /* (58) ":" [n/a] */ 0x89, /* (59) ";" [n/a] */ 0x89, /* (60) "<" [n/a] */ 0x89, 
	/* (61) "=" */ 0x48, /* (62) ">" [n/a] */ 0x89, /* (63) "?" */ 0x83, /* (64) "@" */ 0xFB, /* (65) "A" */ 0x77, 
	/* (66) "B" [cnf] */ 0x7F, /* (67) "C" */ 0x39, /* (68) "D" [cnf] */ 0x3F, /* (69) "E" */ 0x79, /* (70) "F" */ 0x71, 
	/* (71) "G" */ 0x7D, /* (72) "H" */ 0x76, /* (73) "I" */ 0x06, /* (74) "J" */ 0x1E, /* (75) "K" [n/a] */ 0x89, 
	/* (76) "L" */ 0x38, /* (77) "M" [n/a] */ 0x89, /* (78) "N" [n/a] */ 0x54, /* (79) "O" [cnf] */ 0x3F, /* (80) "P" */ 0x73, 
	/* (81) "Q" */ 0xBF, /* (82) "R" [cnf] */ 0x77, /* (83) "S" [cnf] */ 0x6D, /* (84) "T" [n/a] */ 0x89, /* (85) "U" */ 0x3E, 
	/* (86) "V" [n/a] */ 0x89, /* (87) "W" [n/a] */ 0x89, /* (88) "X" [cnf] */ 0x76, /* (89) "Y" */ 0x6E, /* (90) "Z" [cnf] */ 0x5B, 
	/* (91) "[" [cnf] */ 0x39, /* (92) "\" */ 0x64, /* (93) "]" [cnf] */ 0x0F, /* (94) "^" */ 0x23, /* (95) "_" */ 0x88, 
	/* (96) "`" */ 0x20, /* (97) "a" */ 0xDF, /* (98) "b" */ 0x7C, /* (99) "c" */ 0x58, /* (100) "d" */ 0x5E, 
	/* (101) "e" [n/a] */ 0x79, /* (102) "f" [n/a] */ 0x71, /* (103) "g" */ 0x7D, /* (104) "h" */ 0x74, /* (105) "i" */ 0x10, 
	/* (106) "j" */ 0x1E, /* (107) "k" [n/a] */ 0x89, /* (108) "l" */ 0x86, /* (109) "m" [n/a] */ 0x89, /* (110) "n" */ 0x54, 
	/* (111) "o" */ 0x5C, /* (112) "p" */ 0x73, /* (113) "q" */ 0x67, /* (114) "r" */ 0x50, /* (115) "s" [cnf] */ 0x6D, 
	/* (116) "t" [n/a] */ 0x89, /* (117) "u" */ 0x1C, /* (118) "v" [cnf] */ 0x1C, /* (119) "w" [n/a] */ 0x89, /* (120) "x" */ 0xB6, 
	/* (121) "y" [cnf] */ 0x6E, /* (122) "z" [cnf] */ 0x5B, /* (123) "{" [cnf] */ 0x39, /* (124) "|" */ 0x30, /* (125) "}" [cnf] */ 0x0F, 
	/* (126) "~" [n/a] */ 0x89, /* (127) "" */ 0x00, /* (128) "" */ 0xFF
};

// table to map ascii to segments on the alphanum displays
// font table from Adafruit.  Verbatim attribution:
/*!
 * @file Adafruit_LEDBackpack.cpp
 *
 * @mainpage Adafruit LED Backpack library for Arduino.
 *
 * @section intro_sec Introduction
 *
 * This is an Arduino library for our I2C LED Backpacks:
 * ----> http://www.adafruit.com/products/
 * ----> http://www.adafruit.com/products/
 *
 * These displays use I2C to communicate, 2 pins are required to
 * interface. There are multiple selectable I2C addresses. For backpacks
 * with 2 Address Select pins: 0x70, 0x71, 0x72 or 0x73. For backpacks
 * with 3 Address Select pins: 0x70 thru 0x77
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * This library depends on <a
 * href="https://github.com/adafruit/Adafruit-GFX-Library"> Adafruit_GFX</a>
 * being present on your system. Please make sure you have installed the latest
 * version before using this library.
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * MIT license, all text above must be included in any redistribution
 *
 */
// End verbatim attribution

static const uint16_t ht16k33_alphanum[] = {
    0b0000000000000001,
    0b0000000000000010,
    0b0000000000000100,
    0b0000000000001000,
    0b0000000000010000,
    0b0000000000100000,
    0b0000000001000000,
    0b0000000010000000,
    0b0000000100000000,
    0b0000001000000000,
    0b0000010000000000,
    0b0000100000000000,
    0b0001000000000000,
    0b0010000000000000,
    0b0100000000000000,
    0b1000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0001001011001001,
    0b0001010111000000,
    0b0001001011111001,
    0b0000000011100011,
    0b0000010100110000,
    0b0001001011001000,
    0b0011101000000000,
    0b0001011100000000,
    0b0000000000000000, //
    0b0000000000000110, // !
    0b0000001000100000, // "
    0b0001001011001110, // #
    0b0001001011101101, // $
    0b0000110000100100, // %
    0b0010001101011101, // &
    0b0000010000000000, // '
    0b0010010000000000, // (
    0b0000100100000000, // )
    0b0011111111000000, // *
    0b0001001011000000, // +
    0b0000100000000000, // ,
    0b0000000011000000, // -
    0b0000000000000000, // .
    0b0000110000000000, // /
    0b0000110000111111, // 0
    0b0000000000000110, // 1
    0b0000000011011011, // 2
    0b0000000010001111, // 3
    0b0000000011100110, // 4
    0b0010000001101001, // 5
    0b0000000011111101, // 6
    0b0000000000000111, // 7
    0b0000000011111111, // 8
    0b0000000011101111, // 9
    0b0001001000000000, // :
    0b0000101000000000, // ;
    0b0010010000000000, // <
    0b0000000011001000, // =
    0b0000100100000000, // >
    0b0001000010000011, // ?
    0b0000001010111011, // @
    0b0000000011110111, // A
    0b0001001010001111, // B
    0b0000000000111001, // C
    0b0001001000001111, // D
    0b0000000011111001, // E
    0b0000000001110001, // F
    0b0000000010111101, // G
    0b0000000011110110, // H
    0b0001001000001001, // I
    0b0000000000011110, // J
    0b0010010001110000, // K
    0b0000000000111000, // L
    0b0000010100110110, // M
    0b0010000100110110, // N
    0b0000000000111111, // O
    0b0000000011110011, // P
    0b0010000000111111, // Q
    0b0010000011110011, // R
    0b0000000011101101, // S
    0b0001001000000001, // T
    0b0000000000111110, // U
    0b0000110000110000, // V
    0b0010100000110110, // W
    0b0010110100000000, // X
    0b0001010100000000, // Y
    0b0000110000001001, // Z
    0b0000000000111001, // [
    0b0010000100000000, //
    0b0000000000001111, // ]
    0b0000110000000011, // ^
    0b0000000000001000, // _
    0b0000000100000000, // `
    0b0001000001011000, // a
    0b0010000001111000, // b
    0b0000000011011000, // c
    0b0000100010001110, // d
    0b0000100001011000, // e
    0b0000000001110001, // f
    0b0000010010001110, // g
    0b0001000001110000, // h
    0b0001000000000000, // i
    0b0000000000001110, // j
    0b0011011000000000, // k
    0b0000000000110000, // l
    0b0001000011010100, // m
    0b0001000001010000, // n
    0b0000000011011100, // o
    0b0000000101110000, // p
    0b0000010010000110, // q
    0b0000000001010000, // r
    0b0010000010001000, // s
    0b0000000001111000, // t
    0b0000000000011100, // u
    0b0010000000000100, // v
    0b0010100000010100, // w
    0b0010100011000000, // x
    0b0010000000001100, // y
    0b0000100001001000, // z
    0b0000100101001001, // {
    0b0001001000000000, // |
    0b0010010010001001, // }
    0b0000010100100000, // ~
    0b0011111111111111
};
					    



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
int HT16K33_UPDATE_RAW(HT16K33 *backpack, unsigned short digit, const uint16_t value);

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
