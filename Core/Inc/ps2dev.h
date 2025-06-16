/*
 * ps2dev.h
 *
 *  Created on: Jun 13, 2025
 *      Author: pjw
 */

#ifndef INC_PS2DEV_H_
#define INC_PS2DEV_H_

#include <stdint.h>

#include "main.h"

extern void ps2_init(GPIO_TypeDef * port_in, int clk_in, int data_in, GPIO_TypeDef * port_out, int clk_out, int data_out);
extern int ps2_write(uint8_t data);
extern int ps2_read(uint8_t * data);
extern int ps2_available();
extern void ps2_keyboard_init();
extern void ps2_keyboard_scan_code_set(short set);
extern short ps2_keyboard_get_set();
extern int ps2_keyboard_press(uint8_t code);
extern int ps2_keyboard_release(uint8_t code);
extern int ps2_keyboard_press_special(uint8_t code);
extern int ps2_keyboard_release_special(uint8_t code);
extern int ps2_keyboard_press_printscreen();
extern int ps2_keyboard_release_printscreen();
extern int ps2_keyboard_mkbrk_printscreen();
extern int ps2_keyboard_pausebreak();
extern int ps2_keyboard_reply(uint8_t cmd, uint8_t *leds);
extern int ps2_keyboard_handle(uint8_t *leds);
extern int ps2_keyboard_mkbrk(uint8_t code);
extern int ps2_keyboard_special_mkbrk(uint8_t code);

#endif /* INC_PS2DEV_H_ */
