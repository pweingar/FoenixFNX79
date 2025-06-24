/*
 * ws2812.h
 *
 *  Created on: Jun 21, 2025
 *      Author: pjw
 */

#ifndef INC_WS2812_H_
#define INC_WS2812_H_

#include <stdbool.h>
#include <stdint.h>

// RGB Status LED addresses
// TODO: Fix the addresses once the off-by-one issue is fixed
#define LED_FUNC_BASE		(4)
#define LED_CAPS			(3)
#define LED_NETWORK			(2)
#define LED_MEDIA			(1)
#define LED_POWER			(0)

/**
 * Flag that the WS2812s need to be updated with the current color values
 */
extern void ws2812_trigger();

/**
 * Set the color of a given LED in the records, but do not actually update the LEDs
 *
 * @param led the index of the LED to update (0 -- 11)
 * @param red the red component 0 -- 255
 * @param green the green component 0 -- 255
 * @param blue the blue component 0 -- 255
 */
extern void ws2812_set_color(unsigned short led, uint8_t red, uint8_t green, uint8_t blue);

/**
 * Set whether or not an LED is on
 *
 * @param led the index of the LED to update (0 -- 11)
 * @param is_on TRUE if the LED should be on, FALSE if it should be off
 */
extern void ws2812_set_state(short led, bool is_on);

/**
 * Write the byte to the PWM buffer
 *
 * @param buffer pointer to the PWM buffer to write to
 * @param data the byte to write
 * @retval pointer to the next word in the PWM buffer
 */
extern uint32_t * ws2812_buffer_byte(uint32_t * buffer, uint8_t data);

/**
 * Dim the LEDs besides the power LED
 *
 * @param millis the current number of milliseconds
 * @param max the number of millisecond at which the leds should be off
 */
extern void ws2812_dim(long millis, long max);

/**
 * Update the physical LEDs with the set colors
 */
extern void ws2812_update();

/**
 * Initialize the variables for the WS2812 RGB LEDs
 */
extern void ws2812_init();

#endif /* INC_WS2812_H_ */
