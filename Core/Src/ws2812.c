/*
 * ws2818.c
 *
 *  Created on: Jun 21, 2025
 *      Author: pjw
 */

#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "ws2812.h"

//
// Types
//

/**
 * Structure to represent a WS2812 LED's color
 */
typedef struct pixel_rgb_s {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	bool is_on;
} pixel_rgb_t, *pixel_rgb_p;

//
// Constants
//

#define NUM_PIXELS		(12)
#define BYTE_PER_PIXEL	(3)
#define WRITE_BUF_LEN	(NUM_PIXELS * BYTE_PER_PIXEL * 2)


// WS2812 duty cycle timing constants for 0 and 1
const uint8_t NEOPIXEL_ZERO = 12;
const uint8_t NEOPIXEL_ONE = 30;

// Status bits to indicate if the LED transfer DMA cycle is running
const uint8_t DMA_STATE_RESTING = 0;
const uint8_t DMA_STATE_SENDING = 1;

/**
 * Default colors for the RGB LEDs at start time
 */
const pixel_rgb_t led_defaults[] = {
		{0, 255, 0, true},			// power is green
		{0, 0, 255, true},			// LED 1 and 2 are blue
		{0, 0, 255, true},
		{255, 0, 255, true},		// CAPS is purple
		{255, 0, 0, true},			// F1 = red
		{255, 128, 0, true},		// F2 = orange
		{255, 255, 0, true},		// F3 = yellow
		{0, 255, 0, true},			// F4 = green
		{0, 255, 128, true},		// F5 = cyan
		{0, 0, 255, true},			// F6 = blue
		{128, 0, 255, true},		// F7 = magenta?
		{255, 0, 255, true}			// F8 = purple
};

//
// Module variables
//

static pixel_rgb_t pixel[NUM_PIXELS];
static uint32_t dma_buffer[WRITE_BUF_LEN];
static short dma_led_index = 0; // led_dma_index
static bool ws2812_needs_update = false;

//
// Code
//

/**
 * Flag that the WS2812s need to be updated with the current color values
 */
void ws2812_trigger() {
	ws2812_needs_update = true;
}

/**
 * Set the color of a given LED in the records, but do not actually update the LEDs
 *
 * @param led the index of the LED to update (0 -- 11)
 * @param red the red component 0 -- 255
 * @param green the green component 0 -- 255
 * @param blue the blue component 0 -- 255
 */
void ws2812_set_color(unsigned short led, uint8_t red, uint8_t green, uint8_t blue) {
	if (led < NUM_PIXELS) {
		pixel[led].r = red;
		pixel[led].g = green;
		pixel[led].b = blue;
	}
}

/**
 * Set whether or not an LED is on
 *
 * @param led the index of the LED to update (0 -- 11)
 * @param is_on TRUE if the LED should be on, FALSE if it should be off
 */
void ws2812_set_state(short led, bool is_on) {
	if (led < NUM_PIXELS) {
		pixel[led].is_on = is_on;
	}
}

/**
 * Write the byte to the PWM buffer
 *
 * @param buffer pointer to the PWM buffer to write to
 * @param data the byte to write
 * @retval pointer to the next word in the PWM buffer
 */
uint32_t * ws2812_buffer_byte(uint32_t * buffer, uint8_t data) {
	uint8_t mask = 0x80;
	for (short i = 0; i < 8; i++) {
		if (data & mask) {
			*buffer++ = NEOPIXEL_ONE;
		} else {
			*buffer++ = NEOPIXEL_ZERO;
		}

		mask = mask >> 1;
	}

	return buffer;
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	uint32_t * buffer = dma_buffer + (WRITE_BUF_LEN / 2);

	if (dma_led_index < 0) {
		// For the first few pixels, just leave things blank
		dma_led_index++;

	} else if (dma_led_index < NUM_PIXELS) {
		// We're in. Fill the even buffer
		if (pixel[dma_led_index].is_on) {
			buffer = ws2812_buffer_byte(buffer, pixel[dma_led_index].g);
			buffer = ws2812_buffer_byte(buffer, pixel[dma_led_index].r);
			buffer = ws2812_buffer_byte(buffer, pixel[dma_led_index].b);

		} else {
			for (uint8_t i = 0; i < WRITE_BUF_LEN / 2; i++) {
				buffer[i] = 0;
			}
		}

		dma_led_index++;

	} else if (dma_led_index < NUM_PIXELS + 2) {
		// Last two transfers are resets. 64 * 1.25 us = 80 us == good enough reset
		// First half reset zero fill
		for (uint8_t i = 0; i < WRITE_BUF_LEN / 2; i++) {
			buffer = ws2812_buffer_byte(buffer, 0);
			buffer = ws2812_buffer_byte(buffer, 0);
			buffer = ws2812_buffer_byte(buffer, 0);
		}

		dma_led_index++;

	} else {
		HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_2);
	}
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
	uint32_t * buffer = dma_buffer;

	if (dma_led_index < 0) {
		// For the first few pixels, just leave things blank
		dma_led_index++;

	} else if (dma_led_index < NUM_PIXELS) {
		// We're in. Fill the even buffer
		if (pixel[dma_led_index].is_on) {
			buffer = ws2812_buffer_byte(buffer, pixel[dma_led_index].g);
			buffer = ws2812_buffer_byte(buffer, pixel[dma_led_index].r);
			buffer = ws2812_buffer_byte(buffer, pixel[dma_led_index].b);

		} else {
			for (uint8_t i = 0; i < WRITE_BUF_LEN / 2; i++) {
				buffer = ws2812_buffer_byte(buffer, 0);
				buffer = ws2812_buffer_byte(buffer, 0);
				buffer = ws2812_buffer_byte(buffer, 0);
			}
		}

		dma_led_index++;

	} else if (dma_led_index < NUM_PIXELS + 2) {
		// Last two transfers are resets. 64 * 1.25 us = 80 us == good enough reset
		// First half reset zero fill
		for (uint8_t i = 0; i < WRITE_BUF_LEN / 2; i++) {
			dma_buffer[i] = 0;
		}

		dma_led_index++;
	}
}

/**
 * Update the physical LEDs with the set colors
 */
void ws2812_update() {
	if (ws2812_needs_update) {
		ws2812_needs_update = false;

		// Fill the DMA buffer with NIL to force a reset
		for (short i = 0; i < WRITE_BUF_LEN; i++) {
			dma_buffer[i] = 0;
		}

		// We'll send 10 pixels' worth of NILs, which should be enough to be seen as a RESET
		dma_led_index = -10;

		// Start the DMA
		HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t *)dma_buffer, WRITE_BUF_LEN);
	}
}

/**
 * Dim the LEDs besides the power LED
 *
 * @param millis the current number of milliseconds
 * @param max the number of millisecond at which the leds should be off
 */
void ws2812_dim(long millis, long max) {
	if (millis < max) {
		float factor = (float)(max - millis) / (float)max;
		for (short i = 1; i < NUM_PIXELS; i++) {
			ws2812_set_color(i,
					(int)(led_defaults[i].r * factor),
					(int)(led_defaults[i].g * factor),
					(int)(led_defaults[i].b * factor));
		}
	} else {
		for (short i = 1; i < NUM_PIXELS; i++) {
			ws2812_set_color(i, led_defaults[i].r, led_defaults[i].g, led_defaults[i].b);
			pixel[i].is_on = false;
		}
	}

	ws2812_trigger();
}

/**
 * Initialize the variables for the WS2812 RGB LEDs
 */
void ws2812_init() {
	for (short i = 0; i < NUM_PIXELS; i++) {
		pixel[i] = led_defaults[i];
	}

	ws2812_trigger();
}
