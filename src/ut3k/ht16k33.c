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

/* apt-get install libi2c-dev */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <i2c/smbus.h>

#include "ht16k33.h"
#include "ht16k33_lookup_tables.h"

/**
 * write one byte to i2c bus
 */
static inline int ht16k33_write8(HT16K33 *backpack, uint8_t val) {
	if(backpack->adapter_fd == -1) { // not opened
		// TODO: choose a valid errno error
		backpack->lasterr = -1;
		return -1;
	}
	
	//backpack->buf[0] = val;
	if(write(backpack->adapter_fd, &val, sizeof(uint8_t)) != 1) {
		backpack->lasterr = errno;
		return -1;
	}

	return 0;
}
/**
 * Convert a ascii char to it's 7 segment display binary representation.
 * 
 * WARNING: Due to the hardware code, this one is valid only for the adafruit 7 segment display!!!
 * It won't work with different electronic schematics.
 * 
 * The parameter "x" is the value to display (NOTE: not all characters are valid on a 7 seg display!!)
 * The parameter decimal point can be 0 (decimal point led OFF) or 1 (decimal point led ON)
 */
static inline uint8_t ascii_to_7seg(unsigned int x, unsigned short decimal_point) {
	uint8_t seg7;
	
	if(x > 128) return 0x89; // char not available
	
	seg7 = ht16k33_7seg_digits[x];

	if (decimal_point == 1) seg7 |= ht16k33_7seg_digits[',']; // add the comma
	
	return seg7;
}


/** HT16K33_init
 * useful if the macro version isn't available for the context
 */
void HT16K33_init(HT16K33 *backpack, int i2cadapter, uint8_t driveraddr) {
  backpack->adapter_fd = -1;
  backpack->adapter_nr = i2cadapter;
  backpack->driver_addr = driveraddr;
  backpack->lasterr = 0;
  backpack->interrupt_mode = HT16K33_ROW15_DRIVER;
  backpack->display_state = HT16K33_DISPLAY_OFF;
  backpack->blink_state = HT16K33_BLINK_OFF;
  backpack->brightness = HT16K33_BRIGHTNESS_15;
}



// HT16K33 Functions (inspired on Adafruit_LEDBackpack.cpp at https://github.com/adafruit/Adafruit-LED-Backpack-Library)
int HT16K33_OPEN(HT16K33 *backpack) {
	char filename[20];
	unsigned short i;
	
	if(backpack->adapter_fd != -1) { // already opened
		// TODO: choose a valid errno error
		backpack->lasterr = -1;
		return -1;
	}
	snprintf(filename, 20, "/dev/i2c-%d", backpack->adapter_nr);
	if ((backpack->adapter_fd = open(filename, O_RDWR)) < 0) { // open the device file (requests i2c-dev kernel module loaded)
		backpack->lasterr = errno;
		return -1;
	}
	
	if (ioctl(backpack->adapter_fd, I2C_SLAVE, backpack->driver_addr) < 0) { // talk to the requested device
		backpack->lasterr = errno;
		close(backpack->adapter_fd);
		backpack->adapter_fd = -1;
		return -1;
	}
	
	
	// clean all 8 com lines
	for (i=0; i<8; i++) {
	  HT16K33_CLEAN_DIGIT(backpack, i);
	}
	
	return 0;
}
void HT16K33_CLOSE(HT16K33 *backpack) {
	if(backpack->adapter_fd != -1) {
		close(backpack->adapter_fd);
		backpack->adapter_fd = -1;
	}
	HT16K33_OFF(backpack);
}
// wake up HT16K33 by setting the S option bit of the "system setup register" (that starts at 0x20h. ON: 0x21 (00100001), OFF: 0x20 (00100000))
int HT16K33_ON(HT16K33 *backpack) {
	int rc;
	
	if(backpack->adapter_fd == -1) { // not opened
		// TODO: choose a valid errno error
		backpack->lasterr = -1;
		return -1;
	}
	
	// check if the display is ON (in this case the display is not in power saving mode, and we can send the command)
	if((HT16K33_DISPLAY_ON & backpack->display_state) == HT16K33_DISPLAY_ON) { // already on
		rc = 0;
	} else { // it's off. Wake up!
		rc = ht16k33_write8(backpack, HT16K33_WAKEUP_OP);
		if(rc == 0) backpack->display_state = HT16K33_DISPLAY_ON;
	}
	return rc;
}
// turn HT16K33 into power saving mode
int HT16K33_OFF(HT16K33 *backpack) {
	int rc;
	
	if(backpack->adapter_fd == -1) { // not opened
		// TODO: choose a valid errno error
		backpack->lasterr = -1;
		return -1;
	}
	
	// check if the display is ON (in this case the display is not in power saving mode, and we can send the command)
	if((HT16K33_DISPLAY_ON & backpack->display_state) == HT16K33_DISPLAY_ON) { // it's on
		// go to sleep
		rc = ht16k33_write8(backpack, HT16K33_SLEEP_OP);
		if(rc == 0) backpack->display_state = HT16K33_DISPLAY_OFF;
	} else { // already off
		rc = 0;
	}
	return rc;
}

/**
 * set interrupt mode or turn off for row15
 */
int HT16K33_INTERRUPT(HT16K33 *backpack, ht16k33interrupt_t interrupt_mode) {
	int rc = -1;
	uint8_t cmd;
	
	if(backpack->adapter_fd == -1) {
		backpack->lasterr = rc;
		return rc;
	}

	/* "display setup register" command is composed as:
	 * MSB             LSB
	 * 1 0 1 0 X X action row/int
	 * 
	 * X == don't care
	 * action: active low (0) active high (1)
	 * row/int: row driver (0) interrupt (1)
	 */
	cmd = HT16K33_INTERRUPT_BASE | interrupt_mode;

	// <<guessing>> this shouldn't be fiddled with if the display is on
	// if ((HT16K33_DISPLAY_ON & backpack->display_state) == HT16K33_DISPLAY_ON) {
	//	  return rc;
	//}
	//else {
	  rc = ht16k33_write8(backpack, cmd);
	  //}

	if (rc == 0) {
	  // store that we set it
	  backpack->interrupt_mode = interrupt_mode;
	}
	return rc;
}



/**
 * blink_cmd can be one of HT16K33_BLINK_OFF, HT16K33_BLINK_SLOW (0.5 HZ), HT16K33_BLINK_NORMAL (1 HZ), HT16K33_BLINK_FAST (2 HZ)
 */
int HT16K33_BLINK(HT16K33 *backpack, ht16k33blink_t blink_cmd) {
	int rc=-1;
	uint8_t cmd;
	ht16k33blink_t new_blink_state;
	
	if(backpack->adapter_fd == -1) {
		backpack->lasterr = -1;
		return -1;
	}
	
	/* "display setup register" command is composed as:
	 * MSB             LSB
	 * 1 0 0 0 0 B1 B0 D
	 * 
	 * B1-B0 defines the blinking frequency
	 * D defined display on/off status
	 */
	
	// no blink, display off
	cmd = HT16K33_DISPLAY_SETUP_BASE;
	
	switch(blink_cmd) {
	case HT16K33_BLINK_OFF:
		cmd |= HT16K33_BLINK_OFF | backpack->display_state;
		new_blink_state = HT16K33_BLINK_OFF;
		break;
	case HT16K33_BLINK_SLOW:
		cmd |= HT16K33_BLINK_SLOW | backpack->display_state;
		new_blink_state = HT16K33_BLINK_SLOW;
		break;
	case HT16K33_BLINK_NORMAL:
		cmd |= HT16K33_BLINK_NORMAL | backpack->display_state;
		new_blink_state = HT16K33_BLINK_NORMAL;
		break;
	case HT16K33_BLINK_FAST:
		cmd |= HT16K33_BLINK_FAST | backpack->display_state;
		new_blink_state = HT16K33_BLINK_FAST;
		break;
	default:
		cmd |= HT16K33_BLINK_NORMAL | backpack->display_state;
		new_blink_state = HT16K33_BLINK_NORMAL;
		break;
	}
	
	// check if the display is ON (in this case the display is NOT in power saving mode, and we can send the command)
	if((HT16K33_DISPLAY_ON & backpack->display_state) == HT16K33_DISPLAY_ON) {
		rc = ht16k33_write8(backpack, cmd);
	} else {
		rc = 0;
	}
	
	if (rc == 0) {
		backpack->blink_state = new_blink_state;
	}
	
	return rc;
}
/**
 * display_cmd can be one of HT16K33_DISPLAY_OFF or HT16K33_DISPLAY_ON
 */
int HT16K33_DISPLAY(HT16K33 *backpack, ht16k33display_t display_cmd) {
	int rc=-1;
	uint8_t cmd;
	ht16k33display_t new_display_state;
	
	if(backpack->adapter_fd == -1) {
		backpack->lasterr = -1;
		return -1;
	}
	
	/* "display setup register" command is composed as:
	 * MSB             LSB
	 * 1 0 0 0 0 B1 B0 D
	 * 
	 * B1-B0 defines the blinking frequency
	 * D defined display on/off status
	 */
	
	// no blink, display off
	cmd = HT16K33_DISPLAY_SETUP_BASE;
	
	switch(display_cmd) {
	case HT16K33_DISPLAY_OFF:
		cmd |= backpack->blink_state | HT16K33_DISPLAY_OFF;
		new_display_state = HT16K33_DISPLAY_OFF;
		break;
	case HT16K33_DISPLAY_ON:
		cmd |= backpack->blink_state | HT16K33_DISPLAY_ON;
		new_display_state = HT16K33_DISPLAY_ON;
		break;
	default:
		cmd |= backpack->blink_state | HT16K33_DISPLAY_OFF;
		new_display_state = HT16K33_DISPLAY_OFF;
		break;
	}
	
	// check if the display is ON (in this case the display is NOT in power saving mode, and we can send the command)
	if((HT16K33_DISPLAY_ON & backpack->display_state) == HT16K33_DISPLAY_ON) {
		rc = ht16k33_write8(backpack, cmd);
	} else {
		rc = 0;
	}
	
	if(rc == 0) {
		backpack->display_state = new_display_state;
	}
	
	return rc;
}
int HT16K33_BRIGHTNESS(HT16K33 *backpack, ht16k33brightness_t brightness) {
	int rc=-1;
	uint8_t cmd;
	//unsigned char buf[16];
	//unsigned char x;
	
	if(backpack->adapter_fd == -1) {
		backpack->lasterr = -1;
		return -1;
	}
	
	if(brightness > 0x0F) brightness = 0x0F; // maximum brightness cannot exceed low nibble
	
	cmd = HT16K33_DIMMING_BASE | brightness;
	// check if the display is ON (in this case the display is NOT in power saving mode, and we can send the command)
	if((HT16K33_DISPLAY_ON & backpack->display_state) == HT16K33_DISPLAY_ON) {
		rc = ht16k33_write8(backpack, cmd);
	} else {
		rc = 0;
	}
	
	if(rc == 0) {
		backpack->brightness = brightness;
	}
	
	return rc;
}
/**
 * Update a single display digit, identified by the digit parameter, starting from 0.
 * The digit 2 is the colon sign in the middle of the four digits.
 * To make it reflected on the 7 segment display you have to call HT16K33_COMMIT()
 */
int HT16K33_UPDATE_DIGIT(HT16K33 *backpack, unsigned short digit, const unsigned char value, unsigned short decimal_point) {
	// digit 0 is the first digit on the left
	// digit 1 is the second digit on the left
	// digit 2 is the colon sign in the middle
	// digit 3 is the third digit, on the right
	// digit 4 is the last digit, on the right
	
	if (digit > 4) { // we are working with a 7 segment, 4 digits display. Only the first 5 columns are used to drive the display
		return -1;
	}
	if (value > 128) { // cannot represent the given character on the display
		return -1;
	}
	
	backpack->display_buffer.com[digit*2] = ascii_to_7seg(value, decimal_point);
	backpack->display_buffer.com[digit*2 + 1] = 0;
	return 0;
}


// TODO: decimal point
int HT16K33_UPDATE_ALPHANUM(HT16K33 *backpack, unsigned short digit, const unsigned char value, unsigned short decimal_point) {
  uint16_t alphanum;

  if (digit > 7) {
    return -1;
  }

  if (value > 128) { // cannot represent the given character on the display
    return -1;
  }

  alphanum = ht16k33_alphanum[value];
  backpack->display_buffer.com[2*digit] = (uint8_t)alphanum;
  backpack->display_buffer.com[2*digit + 1] = (uint8_t) (alphanum >> 8);

  return 0;
}


int HT16K33_UPDATE_RAW(HT16K33 *backpack, const uint16_t value[]) {
  for (int digit = 0; digit < 4; ++digit) {
    backpack->display_buffer.com[2*digit] = (uint8_t) value[digit];
    backpack->display_buffer.com[2*digit+1] = (uint8_t) ((value[digit]) >> 8);
  }
  return 0;
}

int HT16K33_UPDATE_RAW_BYDIGIT(HT16K33 *backpack, unsigned short digit, const uint16_t value) {
  backpack->display_buffer.com[2*digit] = (uint8_t) value;
  backpack->display_buffer.com[2*digit+1] = (uint8_t) (value >> 8);
  return 0;
}


// should probably dump the special handling for 8 bit ints and stick
// with the snprintf branch for all.  The snprintf is slower by 10~50
// microseconds per iteration, but the other version require a big
// table plus my code is likely buggy.
int HT16K33_DISPLAY_INTEGER(HT16K33 *backpack, int16_t value) {
  int digit;
  uint8_t value8 = value;

  // test for an 8 bit value.
  // this legitemately won't display the value 65280 correctly, but
  // that's greater than what we can show on a 4 digit display
  // anyway, so I'm fine with that.
  if ((value & 0xff00) == 0) {
    // non-negative 0-256
    for (digit = 3; digit > 0; --digit) {
      backpack->display_buffer.com[2*digit] = (uint8_t) ht16k33_int_to_display[value8][digit-1];
      backpack->display_buffer.com[2*digit+1] = (uint8_t) (ht16k33_int_to_display[value8][digit-1] >> 8);
    }
    
    backpack->display_buffer.com[0] = (uint8_t) 0;
    backpack->display_buffer.com[1] = (uint8_t) 0;
  }
  else if (value == (int8_t)value) {
    // negative value: -1 -- -128.  TODO: this ought to work to -255.
    // stop if/when we get to a zero value in the display and use a
    // minus sign instead
    value8 = value * -1;

    for (digit = 3; digit > 0; --digit) {
      if (ht16k33_int_to_display[value8][digit-1] == 0) {
	break;
      }
      backpack->display_buffer.com[2*digit] = (uint8_t) ht16k33_int_to_display[value8][digit-1];
      backpack->display_buffer.com[2*digit+1] = (uint8_t) (ht16k33_int_to_display[value8][digit-1] >> 8);
    }
    
    // precede with a minus sign since it's negative
    backpack->display_buffer.com[2 * digit] = (uint8_t) ht16k33_alphanum['-'];
    backpack->display_buffer.com[2 * digit + 1] = (uint8_t) (ht16k33_alphanum['-'] >> 8);

    // fill in any remaining digits with spaces
    for (digit = digit - 1; digit >= 0; --digit) {
      backpack->display_buffer.com[2 * digit] = 0;
      backpack->display_buffer.com[2 * digit + 1] = 0;
    }
  }
  else {
    // optimize this later...
    unsigned char display[5];
    snprintf((char*)display, 5, "%4.hd", value);
    for (digit = 0; digit < 4; ++digit) {
      backpack->display_buffer.com[2*digit] = (uint8_t)ht16k33_alphanum[display[digit]];
      backpack->display_buffer.com[2*digit + 1] = (uint8_t)(ht16k33_alphanum[display[digit]] >> 8);
    }
    
  }

  return 0;
}



/**
 * Clean a single display digit, identified by the digit parameter, starting from 0.
 * allow for all 8 com lines to be cleared
 */
int HT16K33_CLEAN_DIGIT(HT16K33 *backpack, unsigned short digit) {
	// digit 0 is the first digit on the left
	// digit 1 is the second digit on the left
	// digit 2 is the colon sign in the middle
	// digit 3 is the third digit, on the right
	// digit 4 is the last digit, on the right
	
	if (digit > 7) {
		return -1;
	}
	
	backpack->display_buffer.com[digit*2] = 0x00;
	backpack->display_buffer.com[digit*2 + 1] = 0x00;
	return 0;
}
/**
 * Commit the display buffer data to the 7 segment display, showing the saved data.
 * Data must be saved to the buffer calling HT16K33_UPDATE_DIGIT() before calling this one.
 */
int HT16K33_COMMIT(HT16K33 *backpack) {
  if(backpack->adapter_fd == -1) {
    backpack->lasterr = -1;
    return -1;
  }
	
  // commit data to the i2c bus
  return i2c_smbus_write_i2c_block_data(backpack->adapter_fd, 0x00, 16, backpack->display_buffer.com);
}



/**
 * Read the key data from the HT16K33.
 * Key data is 39 bits of data, returned as 6 bytes.
 *      D7  D6  D5  D4  D3  D2  D1  D0
 * KS0  K8  K7  K6  K5  K4  K3  K2  K1
 * KS0   0   0   0  K13 K12 K11 K10 K9
 *
 * KS1  K8  K7  K6  K5  K4  K3  K2  K1
 * KS1   0   0   0  K13 K12 K11 K10 K9
 *
 * KS2  K8  K7  K6  K5  K4  K3  K2  K1
 * KS2   0   0   0  K13 K12 K11 K10 K9
 *
 * ROWS3~15  K1-K8   K9-16
 * COM1      0x40    0x41
 * COM2      0x42    0x43
 * COM3      0x44    0x45
 *
 * this function reads 6 consecutive bytes starting from 0x40 and
 * copies that to the passed in keyscan buffer.
 *
 * Success: return 0
 * fail: return -1 if no bytes are read; otherwise returns the number of bytes read
 */

int HT16K33_READ(HT16K33 *backpack, ht16k33keyscan_t keyscan) {
  int bytes_read;

  if(backpack->adapter_fd == -1) {
    backpack->lasterr = -1;
    return -1;
  }
  
  // read from the i2c bus
  bytes_read = i2c_smbus_read_i2c_block_data(backpack->adapter_fd, HT16K33_KEY_DATA_RAM_BASE, 6, keyscan);

  if (bytes_read != 6){
    return bytes_read == 0 ? -1 : bytes_read;
  }

  return 0;
}
