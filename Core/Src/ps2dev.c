/*
 * ps2dev.c
 *
 *  Created on: Jun 13, 2025
 *      Author: pjw
 *
 *      Modified for STM32 HAL from the Arduino ps2dev library: https://github.com/harvie/ps2dev
 */


/*
 * ps2dev.cpp - an interface library for ps2 host.
 * limitations:
 *      we do not handle parity errors.
 *      The timing constants are hard coded from the spec. Data rate is
 *         not impressive.
 *      probably lots of room for optimization.
 */

#include <stdbool.h>
#include <stdint.h>

#include "stm32f0xx_hal.h"

#include "ps2dev.h"
#include "main.h"

//Enable serial debug mode?
//#define _PS2DBG Serial

#define BYTE_INTERVAL_MICROS 100

//since for the device side we are going to be in charge of the clock,
//the two defines below are how long each _phase_ of the clock cycle is
#define CLKFULL 	40
// we make changes in the middle of a phase, this how long from the
// start of phase to the when we drive the data line
#define CLKHALF 	20

// Delay between bytes
// I've found i need at least 400us to get this working at all,
// but even more is needed for reliability, so i've put 1000us
#define BYTEWAIT 	1000

// Timeout if computer not sending for 30ms
#define TIMEOUT 	30

#define HIGH		1
#define LOW			0

#define ENOERR		0
#define EABORT		-1
#define ETIMEOUT	-2
#define ECANCEL		-3

const uint8_t keyboard_type = 0xf0;

const uint8_t keyboard_version = 0x01;

/**
 * Initial delay, in milliseconds, indexed by the two bits from the typematic PS/2 command
 */
const short ps2_delay_ms[] = {250, 500, 750, 1000};

/**
 * Repeat delay, in milliseconds, indexed by the 5 bits from the typematic PS/2 command
 */
const short ps2_repeat_ms[] = {
	33,
	37,
	42,
	46,
	48,
	54,
	58,
	63,
	67,
	75,
	83,
	92,
	100,
	109,
	116,
	125,
	133,
	149,
	167,
	182,
	200,
	217,
	233,
	250,
	270,
	303,
	333,
	370,
	400,
	435,
	476,
	500
};

/**
 * Module variables
 */

static GPIO_TypeDef * _ps2port_in;
static GPIO_TypeDef * _ps2port_out;
static int _ps2clk_in;
static int _ps2data_in;
static int _ps2clk_out;
static int _ps2data_out;
static bool handling_io_abort;
static uint8_t leds;
static short scan_code_set;

/*
 * according to some code I saw, these functions will
 * correctly set the clock and data pins for
 * various conditions.  It's done this way so you don't need
 * pullup resistors.
 */
void gohi(int pin) {
	HAL_GPIO_WritePin(_ps2port_out, pin, GPIO_PIN_SET);
}

void golo(int pin) {
	HAL_GPIO_WritePin(_ps2port_out, pin, GPIO_PIN_RESET);
}

/*
 * the clock and data pins can be wired directly to the clk and data pins
 * of the PS2 connector.  No external parts are needed.
 */
void ps2_init(GPIO_TypeDef * port_in, int clk_in, int data_in, GPIO_TypeDef * port_out, int clk_out, int data_out) {
	_ps2port_in = port_in;
	_ps2clk_in = clk_in;
	_ps2data_in = data_in;

	_ps2port_out = port_out;
	_ps2clk_out = clk_out;
	_ps2data_out = data_out;

	gohi(_ps2clk_out);
	gohi(_ps2data_out);
	leds = 0;
	handling_io_abort = false;
}

int digitalRead(int pin) {
	if (HAL_GPIO_ReadPin(_ps2port_in, pin) == GPIO_PIN_SET) {
		return HIGH;
	} else {
		return LOW;
	}
}

void delay(int ms) {
	HAL_Delay(ms);
}

int ps2_write(uint8_t data) {
	uint8_t i;
	uint8_t parity = 1;

#ifdef _PS2DBG
	_PS2DBG.print(F("sending "));
	_PS2DBG.println(data,HEX);
#endif

	golo(_ps2data_out);
	delayMicroseconds(CLKHALF);
	// device sends on falling clock
	golo(_ps2clk_out);	// start bit
	delayMicroseconds(CLKFULL);
	gohi(_ps2clk_out);
	delayMicroseconds(CLKHALF);

	for (i=0; i < 8; i++) {
		if (digitalRead(_ps2clk_in) == LOW) { /* I/O request from host */
			/* Abort */
			gohi(_ps2data_out);
			return EABORT;
		}

		if (data & 0x01) {
			gohi(_ps2data_out);
		} else {
			golo(_ps2data_out);
		}

		delayMicroseconds(CLKHALF);
		golo(_ps2clk_out);
		delayMicroseconds(CLKFULL);
		gohi(_ps2clk_out);
		delayMicroseconds(CLKHALF);

		parity = parity ^ (data & 0x01);
		data = data >> 1;
	}

	// parity bit
	if (parity) {
		gohi(_ps2data_out);
	} else {
		golo(_ps2data_out);
	}

	delayMicroseconds(CLKHALF);
	golo(_ps2clk_out);
	delayMicroseconds(CLKFULL);
	gohi(_ps2clk_out);
	delayMicroseconds(CLKHALF);

	if (digitalRead(_ps2clk_in) == LOW) { /* I/O request from host */
		/* Abort */
		gohi(_ps2data_out);
		return EABORT;
	}

	// stop bit
	gohi(_ps2data_out);
	delayMicroseconds(CLKHALF);
	golo(_ps2clk_out);
	delayMicroseconds(CLKFULL);
	gohi(_ps2clk_out);
	delayMicroseconds(CLKHALF);

	delayMicroseconds(BYTEWAIT);

#ifdef _PS2DBG
	_PS2DBG.print(F("sent "));
	_PS2DBG.println(data,HEX);
#endif

	return ENOERR;
}

int ps2_do_write(uint8_t data) {
	int ret;
	if ((ret = ps2_write(data)) == EABORT && !handling_io_abort) {
		handling_io_abort = true;
		ps2_keyboard_handle(&leds);
		handling_io_abort = false;
	}
	return ret;
}

int ps2_available() {
	//delayMicroseconds(BYTEWAIT);
	return ((digitalRead(_ps2data_in) == LOW) || (digitalRead(_ps2clk_in) == LOW) );
}

int ps2_read(uint8_t * value) {
	unsigned int data = 0x00;
	unsigned int bit = 0x01;

	uint8_t calculated_parity = 1;
	uint8_t received_parity = 0;

	//wait for data line to go low and clock line to go high (or timeout)
	unsigned long waiting_since = millis();
	while((digitalRead(_ps2data_in) != LOW) || (digitalRead(_ps2clk_in) != HIGH)) {
		if (!ps2_available()) return ECANCEL; /* Cancelled */
		if((millis() - waiting_since) > TIMEOUT) return ETIMEOUT;
	}

	delayMicroseconds(CLKHALF);
	golo(_ps2clk_out);
	delayMicroseconds(CLKFULL);
	gohi(_ps2clk_out);
	delayMicroseconds(CLKHALF);

	if (digitalRead(_ps2clk_in) == LOW) { /* I/O request from host */
		/* Abort */
		return EABORT;
	}

	while (bit < 0x0100) {
		if (digitalRead(_ps2data_in) == HIGH) {
			data = data | bit;
			calculated_parity = calculated_parity ^ 1;
		} else {
			calculated_parity = calculated_parity ^ 0;
		}

		bit = bit << 1;

		delayMicroseconds(CLKHALF);
		golo(_ps2clk_out);
		delayMicroseconds(CLKFULL);
		gohi(_ps2clk_out);
		delayMicroseconds(CLKHALF);

		if (digitalRead(_ps2clk_in) == LOW) { /* I/O request from host */
			/* Abort */
			return EABORT;
		}
	}

	// we do the delay at the end of the loop, so at this point we have
	// already done the delay for the parity bit

	// parity bit
	if (digitalRead(_ps2data_in) == HIGH) {
		received_parity = 1;
	}

	// stop bit
	delayMicroseconds(CLKHALF);
	golo(_ps2clk_out);
	delayMicroseconds(CLKFULL);
	gohi(_ps2clk_out);
	delayMicroseconds(CLKHALF);


	delayMicroseconds(CLKHALF);
	golo(_ps2data_out);
	golo(_ps2clk_out);
	delayMicroseconds(CLKFULL);
	gohi(_ps2clk_out);
	delayMicroseconds(CLKHALF);
	gohi(_ps2data_out);


	*value = data & 0x00FF;

#ifdef _PS2DBG
	_PS2DBG.print(F("received data "));
	_PS2DBG.println(*value,HEX);
	_PS2DBG.print(F("received parity "));
	_PS2DBG.print(received_parity,BIN);
	_PS2DBG.print(F(" calculated parity "));
	_PS2DBG.println(received_parity,BIN);
#endif
	if (received_parity == calculated_parity) {
		return ENOERR;
	} else {
		return ECANCEL;
	}
}

int ps2_do_read(uint8_t * value) {
	int ret;
	if ((ret = ps2_read(value)) == EABORT && !handling_io_abort) {
		handling_io_abort = true;
		ps2_keyboard_handle(&leds);
		handling_io_abort = false;
	}
	return ret;
}

void ps2_keyboard_init() {
	scan_code_set = 2;
	delay(200);
	ps2_write(0xAA);
	return;
}

void ps2_ack() {
	delayMicroseconds(BYTE_INTERVAL_MICROS);
	ps2_write(0xFA);
	delayMicroseconds(BYTE_INTERVAL_MICROS);
	return;
}

int ps2_keyboard_reply(uint8_t cmd, uint8_t * leds_) {
	uint8_t val = 0;
	uint8_t led_index = 0;
	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
	short delay_ms = 0;
	short rate_ms = 0;

	switch (cmd) {
		case 0xFF: //reset
			kbd_set_typematic(1000, 100);
			ps2_keyboard_scan_code_set(2);
			kbd_set_enable(true);
			*leds_ = 0;
			ps2_ack();
			//the while loop lets us wait for the host to be ready
			while (ps2_write(0xFA) != 0) delay(1); //send ACK
			while (ps2_write(0xAA) != 0) delay(1); // send BAT_SUCCESS
			break;

		case 0xFE: //resend
			ps2_ack();
			break;

		case 0xF6: //set defaults
			//enter stream mode
			kbd_set_typematic(1000, 100);
			ps2_keyboard_scan_code_set(2);
			kbd_set_enable(true);
			*leds_ = 0;
			ps2_ack();
			break;

		case 0xF5: //disable data reporting
			//FM
			kbd_set_enable(false);
			ps2_ack();
			break;

		case 0xF4: //enable data reporting
			//FM
			kbd_set_enable(true);
			ps2_ack();
			break;

		case 0xF3: //set typematic rate
			ps2_ack();
			if (!ps2_read(&val)) {
				// Extract the delay and repeat codes from the parameter byte
				// And convert them to delays in milliseconds
				delay_ms = ps2_delay_ms[(val >> 5) & 0x03];
				rate_ms = ps2_repeat_ms[val & 0x1f];

				// Set the typematic values and ACK
				kbd_set_typematic(delay_ms, rate_ms);
				ps2_ack();
			}
			break;

		case 0xF2: //get device id
			ps2_ack();
			do {
				// ensure ID gets written, some hosts may be sensitive
				if (ps2_do_write(0xAB) == EABORT) continue;
				// this is critical for combined ports (they decide mouse/kb on this)
				if (ps2_do_write(keyboard_type) == EABORT) continue;
				break;
			} while (!handling_io_abort);
			break;

		case 0xF0: //set scan code set
			ps2_ack();
			if (!ps2_read(&val)) {
				if ((val == 1) || (val == 2)) {
					ps2_keyboard_scan_code_set(val);
					ps2_ack();
				} else if (val == 0) {
					ps2_ack();
					if (scan_code_set == 1) {
						ps2_do_write(0x43);
					} else {
						ps2_do_write(0x41);
					}
				}
			}
			break;

		case 0xE0:
			// FNXKBD-79 only: set RGB LED color
			// Parameters: LED#, RED, GREEN, BLUE
			ps2_ack();
			if (!ps2_read(&led_index)) {
				ps2_ack();
				if (!ps2_read(&red)) {
					ps2_ack();
					if (!ps2_read(&green)) {
						ps2_ack();
						if (!ps2_read(&blue)) {
							ws2812_set_color(led_index, red, green, blue);
							ps2_ack();
						}
					}
				}
			}
			break;

		case 0xE1:
			// FNXKBD-79 only: turn RGB LED on
			// Parameters: LED#
			ps2_ack();
			if(!ps2_read(&led_index)) {
				ws2812_set_state(led_index, true);
				ps2_ack();
			}
			break;

		case 0xE2:
			// FNXKBD-79 only: turn RGB LED off
			// Parameters: LED#
			ps2_ack();
			if(!ps2_read(&led_index)) {
				ws2812_set_state(led_index, false);
				ps2_ack();
			}
			break;

		case 0xEE: //echo
			//ack();
			delayMicroseconds(BYTE_INTERVAL_MICROS);
			ps2_write(0xEE);
			delayMicroseconds(BYTE_INTERVAL_MICROS);
			break;

		case 0xED: //set/reset LEDs
			//ack();
			while (ps2_write(0xFA) != 0) delay(1);
			if(!ps2_read(&leds)) {
				ps2_write(0xFA);
			}
#ifdef _PS2DBG
			_PS2DBG.print("LEDs: ");
			_PS2DBG.println(leds, HEX);
			//digitalWrite(LED_BUILTIN, leds);
#endif
			*leds_ = leds;
			return 1;
			break;
	}
	return 0;
}

int ps2_keyboard_handle(uint8_t *leds_) {
	uint8_t c;  //char stores data received from computer for KBD
	if(ps2_available()) {
		if (!ps2_read(&c)) return ps2_keyboard_reply(c, leds_);
	}
	return 0;
}

void ps2_keyboard_scan_code_set(short set) {
	if ((set == 1) || (set == 2)) {
		// Only support scan code set #1 and #2
		scan_code_set = set;
	}
}

 short ps2_keyboard_get_set() {
	 return scan_code_set;
 }

// Presses then releases one of the non-special characters
int ps2_keyboard_mkbrk(uint8_t code)
{
	do {
		if (ps2_do_write(code) == EABORT) continue;
		break;
	} while (!handling_io_abort);
	do {
		if (ps2_do_write(0xF0) == EABORT) continue;
		if (ps2_do_write(code) == EABORT) continue;
		break;
	} while (!handling_io_abort);

  return 0;
}

// Presses one of the non-special characters
int ps2_keyboard_press(uint8_t code)
{
	do {
		if (ps2_do_write(code) == EABORT) continue;
		break;
	} while (!handling_io_abort);

	return 0;
}

// Releases one of the non-special characters
int ps2_keyboard_release(uint8_t code)
{
	do {
		if (scan_code_set == 1) {
			if (ps2_do_write(code | 0x80) == EABORT) continue;

		} else {
			if (ps2_do_write(0xf0) == EABORT) continue;
			if (ps2_do_write(code) == EABORT) continue;
		}
		break;
	} while (!handling_io_abort);

	return 0;
}

// Presses one of the special characters
int ps2_keyboard_press_special(uint8_t code)
{
	do {
		if (ps2_do_write(0xe0) == EABORT) continue;
		if (ps2_do_write(code) == EABORT) continue;
		break;
	} while (!handling_io_abort);

	return 0;
}

// Releases one of the special characters
int ps2_keyboard_release_special(uint8_t code)
{
	do {
		if (scan_code_set == 1) {
			if (ps2_do_write(0xe0) == EABORT) continue;
			if (ps2_do_write(code & 0x80) == EABORT) continue;

		} else {
			if (ps2_do_write(0xe0) == EABORT) continue;
			if (ps2_do_write(0xf0) == EABORT) continue;
			if (ps2_do_write(code) == EABORT) continue;
		}
		break;
	} while (!handling_io_abort);

	return 0;
}

// Presses then releases one of the special characters
int ps2_keyboard_special_mkbrk(uint8_t code)
{
	do {
		if (ps2_do_write(0xe0) == EABORT) continue;
		if (ps2_do_write(code) == EABORT) continue;
		break;
	} while (!handling_io_abort);
	do {
		if (ps2_do_write(0xe0) == EABORT) continue;
		if (ps2_do_write(0xf0) == EABORT) continue;
		if (ps2_do_write(code) == EABORT) continue;
		break;
	} while (!handling_io_abort);

	return 0;
}

// Presses Printscreen
int ps2_keyboard_press_printscreen()
{
	do {
		if (ps2_do_write(0xe0) == EABORT) continue;
		if (ps2_do_write(0x12) == EABORT) continue;
		if (ps2_do_write(0xe0) == EABORT) continue;
		if (ps2_do_write(0x7c) == EABORT) continue;
		break;
	} while (!handling_io_abort);


	return 0;
}

// Releases Printscreen
int ps2_keyboard_release_printscreen()
{
	do {
		if (ps2_do_write(0xe0) == EABORT) continue;
		if (ps2_do_write(0xf0) == EABORT) continue;
		if (ps2_do_write(0x7c) == EABORT) continue;
		if (ps2_do_write(0xe0) == EABORT) continue;
		if (ps2_do_write(0xf0) == EABORT) continue;
		if (ps2_do_write(0x12) == EABORT) continue;
		break;
	} while (!handling_io_abort);

	return 0;
}

// Presses then releases Printscreen
int ps2_keyboard_mkbrk_printscreen()
{
	ps2_keyboard_press_printscreen();
	ps2_keyboard_release_printscreen();

	return 0;
}

// Presses/Releases Pause/Break
int ps2_keyboard_pausebreak()
{
	do {
		if (ps2_do_write(0xe1) == EABORT) continue;
		if (ps2_do_write(0x14) == EABORT) continue;
		if (ps2_do_write(0x77) == EABORT) continue;
		break;
	} while (!handling_io_abort);
	do {
		if (ps2_do_write(0xe1) == EABORT) continue;
		if (ps2_do_write(0xf0) == EABORT) continue;
		if (ps2_do_write(0x14) == EABORT) continue;
		if (ps2_do_write(0xe0) == EABORT) continue;
		if (ps2_do_write(0x77) == EABORT) continue;
		break;
	} while (!handling_io_abort);

	return 0;
}
