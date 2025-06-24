/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdbool.h>
#include <stdint.h>

#include "ps2dev.h"
#include "ws2812.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

const long led_dim_length_ms = 1500;

//
// PS/2 command and response codes
//

const uint8_t PS2_CMD_SET_LED = 0xed;
const uint8_t PS2_CMD_ECHO = 0xee;
const uint8_t PS2_CMD_SET_SC = 0xf0;
const uint8_t PS2_CMD_IDENT = 0xf2;
const uint8_t PS2_CMD_SET_REPEAT = 0xf3;
const uint8_t PS2_CMD_ENABLE = 0xf4;
const uint8_t PS2_CMD_DISABLE = 0xf5;
const uint8_t PS2_CMD_DEFAULTS = 0xf6;
const uint8_t PS2_CMD_RESEND = 0xfe;
const uint8_t PS2_CMD_RESET = 0xff;

const uint8_t PS2_REPLY_ACK = 0xfa;
const uint8_t PS2_REPLY_RESEND = 0xfe;


/**
 * Bit codes for the various status lights
 * There is no NUMLOCK or SCROLL key on this keyboard, so those bits are ignored.
 */
const uint8_t PS2_STAT_CAPS = 0x04;
const uint8_t PS2_STAT_POWER = 0x10;
const uint8_t PS2_STAT_MEDIA1 = 0x20;
const uint8_t PS2_STAT_MEDIA2 = 0x40;

//
// Keyboard matrix constants
//

#define NUM_KEYS 		128
#define NUM_ROWS 		6
#define NUM_COLUMNS		16

const short HEARTBEAT_PERIOD = 10;	// Number of milliseconds per heart beat
const uint8_t HEARTBEAT_CNT = 10;	// Number of counts of the heart beat before we toggle the LED
const uint8_t DEBOUNCE_CNT = 2;		// Number of times through a loop to verify the switch isn't bouncing

const uint8_t KEY_PRESSED = 0x80;
const uint8_t KEY_NO_MAKE = 0x40;
const uint8_t KEY_NO_BREAK = 0x20;
const uint8_t KEY_NO_REPEAT	= 0x10;

const uint8_t KEY_ACK = 0xFA;
const uint8_t KEY_RESEND = 0xFE;

// Mapping of row number to GPIO port
const GPIO_TypeDef * row_port[] = {
		ROW1_GPIO_Port, ROW2_GPIO_Port, ROW3_GPIO_Port, ROW4_GPIO_Port, ROW5_GPIO_Port, ROW6_GPIO_Port
};

// Mapping of row number to GPIO pin
const uint16_t row_pin[] = {
		ROW1_Pin, ROW2_Pin, ROW3_Pin, ROW4_Pin, ROW5_Pin, ROW6_Pin
};

// Mapping of column number to GPIO port
const GPIO_TypeDef * col_port[] = {
		COL1_GPIO_Port, COL2_GPIO_Port, COL3_GPIO_Port, COL4_GPIO_Port,
		COL5_GPIO_Port, COL6_GPIO_Port, COL7_GPIO_Port, COL8_GPIO_Port,
		COL9_GPIO_Port, COL10_GPIO_Port, COL11_GPIO_Port, COL12_GPIO_Port,
		COL13_GPIO_Port, COL14_GPIO_Port, COL15_GPIO_Port, COL16_GPIO_Port
};

// Mapping of column number to GPIO pin
const uint16_t col_pin[] = {
		COL1_Pin, COL2_Pin, COL3_Pin, COL4_Pin,
		COL5_Pin, COL6_Pin, COL7_Pin, COL8_Pin,
		COL9_Pin, COL10_Pin, COL11_Pin, COL12_Pin,
		COL13_Pin, COL14_Pin, COL15_Pin, COL16_Pin
};

// Mapping of matrix position to Foenix scan code

// Scan Code Set 1 -- NOTE: scancode > 0x90 indicates a code requiring an escape (0xf1 => 0xe0 0x71)
const uint8_t scan_code_1[] = {
		// Row 1: ESC, F1 - F8, HELP
		0x01, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x00, 0x00, 0x40, 0x41, 0x42, 0x00, 0x43, 0x44, 0x00, 0x57,

		// Row 2: "`", 1 - 0, -, =, BS, DEL
		0x29, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x2B, 0x0E, 0xd3,

		// Row 3: TAB, Q, W, E, R, T, Y, U, I, O, P, [, ]
		0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x00, 0x00, 0x00,

		// Row 4: --, CAPS, A, S, D, F, G, H, J, K, L, ;, '
		0x00, 0x3A, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x1C, 0x00,

		// Row 5: SHIFT, Z, X, C, V, B, N, M, ",", ".", "/", SHIFT, UP
		0x2A, 0x5A, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0xc8, 0x00, 0x00,

		// Row 6: CTRL, ALT, FN, META, SPACE, ALT, CTRL, "\", LEFT. DOWN, RIGHT
		0x1D, 0x38, 0xdB, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0xdc, 0xb8, 0x9d, 0xcb, 0xd0, 0xcd
};

// Scan Code Set 2 -- NOTE: scancode > 0x90 indicates a code requiring an escape (0xf1 => 0xe0 0x71)
const uint8_t scan_code_2[] = {
		// Row 1: ESC, F1 - F8, HELP
		0x76, 0x05, 0x06, 0x04, 0x0c, 0x03, 0x00, 0x00, 0x0b, 0x83, 0x0a, 0x00, 0x01, 0x09, 0x00, 0x78,

		// Row 2: "`", 1 - 0, -, =, \, BS, DEL
		0x0e, 0x16, 0x1e, 0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x45, 0x4e, 0x55, 0x5d, 0x66, 0xf1,

		// Row 3: TAB, Q, W, E, R, T, Y, U, I, O, P, [, ]
		0x0d, 0x15, 0x1d, 0x24, 0x2d, 0x2c, 0x35, 0x3c, 0x43, 0x44, 0x4d, 0x54, 0x5b, 0x00, 0x00, 0x00,

		// Row 4: --, CAPS, A, S, D, F, G, H, J, K, L, ;, ', blank, return
		0x00, 0x58, 0x1c, 0x1b, 0x23, 0x2b, 0x34, 0x33, 0x3b, 0x42, 0x4b, 0x4c, 0x52, 0x53, 0x5a, 0x00,

		// Row 5: SHIFT, blank, Z, X, C, V, B, N, M, ",", ".", "/", SHIFT, UP
		0x12, 0x13, 0x1a, 0x22, 0x21, 0x2a, 0x32, 0x31, 0x3a, 0x41, 0x49, 0x4a, 0x59, 0xf5, 0x00, 0x00,

		// Row 6: CTRL, ALT, FN, META, SPACE, ALT, CTRL, "\", LEFT. DOWN, RIGHT
		0x14, 0x11, 0x9f, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0xa7, 0x91, 0x94, 0xeb, 0xf2, 0xf4
};

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim16;
TIM_HandleTypeDef htim17;
DMA_HandleTypeDef hdma_tim1_ch2;

/* USER CODE BEGIN PV */

volatile short is_transmitting = 0;		// Flag to indicate if the controller is currently sending a packet
volatile short key_init_delay;			// Counter for the typematic delay on the last pressed key
volatile short key_repeat_delay;		// Counter for the typematic repeat delay
volatile short send_repeat;				// Flag to indicate if the main loop should send the last pressed character

short delay_initial;					// Delay while a key is held down before it starts repeating (in ms)
short delay_repeat;						// Delay between repeats for the held down key (in ms)
uint8_t last_pressed;					// The scan code of the last (and currently) pressed key

uint8_t is_scan_disabled = 0;			// Scanning is disabled if this is TRUE

short current_set = 2;					// Scan code set to use: 1 or 2 (default)

// The status of the keys:
// 0b1xxxxxxx - Pressed
// 0bx1xxxxxx - Don't send a MAKE scan code if pressed
// 0bxx1xxxxx - Don't send a BREAK scan code if released
// 0bxxx1xxxx - Don't repeat the key (typematic) if held
uint8_t p_key_state[NUM_KEYS];

uint8_t led0_state = 0;					// State of LED0 (0 = off, 1 == on)

long millisecond_count = 0;

int	heartbeat_count = 0;				// Count of loops for the heart beat LED

uint8_t modifier_leds = 0;

static short ws2812_intro_state = 2;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM17_Init(void);
static void MX_TIM16_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim == &htim16) {
		// Increase the millisecond counter
		millisecond_count++;
	}
}

/**
 * Wait for us microseconds
 *
 * @param us the number of microseconds to wait
 */
void delayMicroseconds(uint16_t us) {
	__HAL_TIM_SET_COUNTER(&htim17,0);			// set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim17) < us);	// wait for the counter to reach the us input in the parameter
}

/**
 * Return the number of milliseconds since startup
 */
long millis() {
	return millisecond_count;
}

//
// Status LED code
//

/**
 * Set whether a status LED is on or off
 *
 * @param led the number of the status LED (0 or 1)
 */
void stat_led_set(short led, bool is_on) {
	GPIO_TypeDef * gpio_led = (led == 0) ? STAT_LED0_GPIO_Port : STAT_LED1_GPIO_Port;
	uint16_t pin_led = (led == 0) ? STAT_LED0_Pin : STAT_LED1_Pin;

	if (is_on) {
		HAL_GPIO_WritePin(gpio_led, pin_led, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(gpio_led, pin_led, GPIO_PIN_SET);
	}
}

//
// PS/2 Command code
//

/**
 * Set the status LEDs in response to the set LED command (0xed)
 *
 * @param status the bitfield for all the status LEDs
 */
void ps2_set_leds(uint8_t status) {
	ws2812_set_state(LED_CAPS, ((status & PS2_STAT_CAPS) != 0));
	ws2812_trigger();
}

/**
 * Keyboard matrix code
 */

/**
 * De-assert all the row pins
 */
void clr_rows() {
	for (int i = 0; i < NUM_ROWS; i++) {
		HAL_GPIO_WritePin(row_port[i], row_pin[i], GPIO_PIN_RESET);
	}
}

/**
 * Assert or de-assert a row in the key switch matrix
 *
 * @param row the number of the row to assert (0 - 5)
 * @param assert TRUE (non-zero) to assert the pin, FALSE (0) to de-assert it
 */
void set_row(short row, short assert_row) {
	if ((row >= 0) || (row <= 5)) {
		if (assert_row) {
			// Assert the row by lowering its pin
			HAL_GPIO_WritePin(row_port[row], row_pin[row], GPIO_PIN_SET);

			// Delay a little to allow the row's IR LEDs to turn on and settle
			HAL_Delay(1);
		} else {
			// De-assert the row by raising its pin
			HAL_GPIO_WritePin(row_port[row], row_pin[row], GPIO_PIN_RESET);
		}
	}
}

/**
 * Return TRUE if the indicated column is asserted
 *
 * @param column the number of the column to check (0 - 15)
 *
 * @retval TRUE (non-zero) if the column is asserted, FALSE (0) if not
 */
int is_column_asserted(short column) {
	if ((column >= 0) || (column <= 15)) {
		return (HAL_GPIO_ReadPin(col_port[column], col_pin[column]) == GPIO_PIN_SET);
	} else {
		return 0;
	}
}

/**
 * Set the initial delay and delay rate in milliseconds for the key repeat (typematic) function
 *
 * @param delay_ms the initial delay in milliseconds
 * @param rate_ms the repeat rate delay in milliseconds
 */
void kbd_set_typematic(short delay_ms, short rate_ms) {
	delay_initial = delay_ms;
	delay_repeat = rate_ms;
}

/**
 * Send a key scan code to the host
 *
 * @param key_position the index of the key in the p_key_state table
 * @param is_pressed TRUE (non-zero) if the key is pressed (send a MAKE scan code), 0 if not (send a BREAK scan code)
 */
void send_key(short key_position, short is_pressed) {
	// Get the base scan code given the key position and the current scan code set
	uint8_t code = 0;
	switch (ps2_keyboard_get_set()) {
	case 1:
		code = scan_code_1[key_position];
		break;

	case 2:
		code = scan_code_2[key_position];
		break;

	default:
		break;
	}

	// If we got an actual scan code...
	if (code) {
		if (is_pressed) {
			// If it's pressed, send a press scan code or a special press scan code sequence
			// NOTE: special keys (requiring a sequence) are marked by base scan codes > 0x90
			if (code > 0x90) {
				ps2_keyboard_press_special(code & 0x7f);
			} else {
				ps2_keyboard_press(code);
			}

		} else {
			// If it's released, send a release scan code or a special release scan code sequence
			if (code > 0x90) {
				ps2_keyboard_release_special(code & 0x7f);
			} else {
				ps2_keyboard_release(code);
			}
		}
	}
}

/*
 * Process the a key moving into a new state.
 *
 * Check to see if we can send make/break and send accordingly.
 * Check to see if the key can be repeated, and make it the most recent keypress
 *
 * Inputs:
 * key_position: the index of the key in the p_key_state table
 * is_pressed: TRUE (non-zero) if the key is pressed (send a MAKE scan code), 0 if not (send a BREAK scan code)
 */
void process_key(short key_position, short is_pressed) {
	if (is_pressed) {
		if ((p_key_state[key_position] & KEY_NO_MAKE) == 0) {
			// This key can send a MAKE scan code, so queue one
			send_key(key_position, is_pressed);
		}

		if ((p_key_state[key_position] & KEY_NO_REPEAT) == 0) {
			// This key can repeat, so make it the most recent key
			// Set the delay counters as well
			last_pressed = key_position;
			key_repeat_delay = 0;
			key_init_delay = 0; // millis() + delay_initial;
			// HAL_TIM_Base_Start_IT(&htim16);

		} else {
			// Otherwise, cancel any repeating character
			key_repeat_delay = 0;
			key_init_delay = 0;
			last_pressed = 0;
			// HAL_TIM_Base_Stop_IT(&htim16);
		}

	} else {
		// The key has been released

		if ((p_key_state[key_position] & KEY_NO_BREAK) == 0) {
			// This key can send a BREAK scan code, so queue one
			send_key(key_position, is_pressed);
		}

		// Cancel any repeating character
		key_repeat_delay = 0;
		key_init_delay = 0;
		last_pressed = 0;
		// HAL_TIM_Base_Stop_IT(&htim16);
	}
}

/*
 * Clear the debounce counter for the key
 *
 * Inputs:
 * key_position = the position of the key in the matrix
 *
 */
void key_clear_count(uint8_t key_position) {
	p_key_state[key_position] &= 0xf0;
}

/*
 * Return true if the key has changed but been held in the same position
 * long enough to smooth out switch bouncing
 *
 * Inputs:
 * key_position = the position of the key in the matrix
 *
 * Returns:
 * 0 if the key has changed recently, 1 if the key state has been the same long enough
 */
short key_stable(uint8_t key_position) {
//	short count = p_key_state[key_position] & 0x0f;
//	if (count > DEBOUNCE_CNT) {
//		key_clear_count(key_position);
//		return 1;
//	} else {
//		p_key_state[key_position] = (p_key_state[key_position] & 0xf0) | ((count + 1) & 0x0f);
//		return 0;
//	}
	return true;
}

/*
 * Do the work needed for each loop of the micro controller...
 *
 * Check the matrix for key presses
 * Check for interrupts and process requests from the host
 * Handle timers for typematic
 */
void kbd_process() {
	if (!is_scan_disabled) {
		// First... scan the matrix and process any change in keys
		for (short row = 0; row < NUM_ROWS; row++) {
			set_row(row, 1);

			for (short column = 0; column < NUM_COLUMNS; column++) {
				uint8_t key_position = ((row & 0x07) << 4) | (column & 0x0f);
				if (is_column_asserted(column)) {
					if ((p_key_state[key_position] & KEY_PRESSED) == 0) {
						if (key_stable(key_position)) {
							p_key_state[key_position] |= KEY_PRESSED;
							process_key(key_position, 1);
						}
					} else {
						key_clear_count(key_position);
					}
				} else {
					if ((p_key_state[key_position] & KEY_PRESSED) != 0) {
						if (key_stable(key_position)) {
							p_key_state[key_position] &= ~KEY_PRESSED;
							process_key(key_position, 0);
						}
					} else {
						key_clear_count(key_position);
					}
				}
			}

			set_row(row, 0);
		}

//		// Next... check to see if we need to repeat the last key pressed
//		if (send_repeat) {
//			if (last_pressed != 0) {
//				// Send the key and reset the timers for another repeat
//				send_key(last_pressed, 1);
//				key_repeat_delay = millis() + delay_repeat;
//				key_init_delay = 0;
//				send_repeat = 0;
//
//			} else {
//				// For some reason, we had a repeat flagged, but nothing to send
//				// Just clear all the typematic variables
//				key_repeat_delay = 0;
//				key_init_delay = 0;
//				send_repeat = 0;
//			}
//		}
	}
}

/**
 * Set whether or not keyboard scanning is enabled
 *
 * @param enable true if the keyboard scanning should be enabled, false otherwise
 */
void kbd_set_enable(bool enable) {
	is_scan_disabled = !enable;
}

/**
 * Set the keyboard to its initial state
 */
void kbd_init() {
	// rb_init(&ps2_output);		// Initialize the byte FIFO going to the host computer

	clr_rows();

	for (short key = 0; key < NUM_KEYS; key++) {
		// Not pressed, will repeat, and will send both make and break scan codes
		p_key_state[key] = 0;
	}

	is_transmitting = 0;		// We're not transmitting anything yet

	last_pressed = 0;			// There isn't a currently held key
	key_init_delay = 0;			// We're not waiting for the initial delay
	key_repeat_delay = 0;		// We're not repeating
	send_repeat = 0;			// We don't need to send a repeat
	delay_initial = 1000;		// Default wait: 1000 ms
	delay_repeat = 100;			// Default repeat rate: 10 cps (100 ms between repeats)

	is_scan_disabled = 0;		// Enable scanning

	/* Turn off the two status LEDs */
	stat_led_set(0, false);
	stat_led_set(1, false);

	// transmit(0x8000 | KEY_ACK);	// Send ack
}

/**
 * Count iterations through the main loop... toggle LED0 after a certain number of times
 */
void heart_beat() {
	if (++heartbeat_count > HEARTBEAT_CNT) {
		heartbeat_count = 0;
		if (led0_state == 0) {
			led0_state = 1;
			stat_led_set(1, true);
		} else {
			led0_state = 0;
			stat_led_set(1, false);
		}
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

	uint8_t old_modifier_leds;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM17_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */

  kbd_init();
  ws2812_init();

//  ps2_init();

  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_Base_Start_IT(&htim16);	// millisecond tick for millis() counter
  HAL_TIM_Base_Start(&htim17);		// microsecond timer

  ps2_init(GPIOA, PS2_CLK_IN_Pin, PS2_DATA_IN_Pin, GPIOA, PS2_CLK_OUT_Pin, PS2_DATA_OUT_Pin);
  ps2_keyboard_init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  HAL_Delay(HEARTBEAT_PERIOD);

	  // Adjust the counters for the typematic system
	  if (key_init_delay > 0) {
		  // If initial delay has not been reached, count it down...
		  if (key_init_delay < millis()) {
			  // We have reached zero or less... lock it to zero and trigger a repeat
			  key_init_delay = 0;
			  send_repeat = true;
		  }
	  } else if (key_repeat_delay > 0) {
		  // If repeat delay has not been reached, count it down...
		  if (key_repeat_delay < millis()) {
			  // We have reached zero or less... lock it to zero and trigger a repeat
			  key_repeat_delay = 0;
			  send_repeat = true;
		  }
	  }

	  // Check for commands from the host...
	  old_modifier_leds = modifier_leds;
	  ps2_keyboard_handle(&modifier_leds);
	  if (modifier_leds != old_modifier_leds) {
		  // If the command was issued to change the LEDs, update the RGB leds
		  ps2_set_leds(modifier_leds);
	  }

	  // Check for changes to key presses in the key matrix
	  // Issue messages to the host for any changes
	  kbd_process();

	  switch (ws2812_intro_state) {
	  case 2:
		  if (millis() < led_dim_length_ms) {
			  ws2812_dim(millis(), led_dim_length_ms);
	  	  } else {
	  		ws2812_intro_state = 1;
	  	  }
		  ws2812_trigger();
		  break;

	  case 1:
		  for (short i = 1; i < 12; i++) {
			  ws2812_set_state(i, false);
		  }
		  ws2812_intro_state = 0;
		  ws2812_trigger();
		  break;

	  default:
		  break;
	  }

	  // Send the changes to the RGB LEDs out to the WS2812s
	  ws2812_update();

	  // Flash the status LEDs to indicate that this main loop is still running
	  heart_beat();

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 59;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_SET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED3;
  htim3.Init.Period = 2000 - 1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 48 - 1;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 1000 - 1;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */

}

/**
  * @brief TIM17 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM17_Init(void)
{

  /* USER CODE BEGIN TIM17_Init 0 */

  /* USER CODE END TIM17_Init 0 */

  /* USER CODE BEGIN TIM17_Init 1 */

  /* USER CODE END TIM17_Init 1 */
  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 47;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 65535;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM17_Init 2 */

  /* USER CODE END TIM17_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin
                          |ROW5_Pin|ROW6_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, PS2_CLK_OUT_Pin|PS2_DATA_OUT_Pin|STAT_LED1_Pin|STAT_LED0_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : ROW1_Pin ROW2_Pin ROW3_Pin ROW4_Pin
                           ROW5_Pin ROW6_Pin */
  GPIO_InitStruct.Pin = ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin
                          |ROW5_Pin|ROW6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PS2_CLK_IN_Pin PS2_DATA_IN_Pin */
  GPIO_InitStruct.Pin = PS2_CLK_IN_Pin|PS2_DATA_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PS2_CLK_OUT_Pin PS2_DATA_OUT_Pin */
  GPIO_InitStruct.Pin = PS2_CLK_OUT_Pin|PS2_DATA_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : COL1_Pin COL2_Pin COL3_Pin COL11_Pin
                           COL12_Pin COL13_Pin COL14_Pin COL15_Pin
                           COL16_Pin COL4_Pin COL5_Pin COL6_Pin
                           COL7_Pin COL8_Pin COL9_Pin COL10_Pin */
  GPIO_InitStruct.Pin = COL1_Pin|COL2_Pin|COL3_Pin|COL11_Pin
                          |COL12_Pin|COL13_Pin|COL14_Pin|COL15_Pin
                          |COL16_Pin|COL4_Pin|COL5_Pin|COL6_Pin
                          |COL7_Pin|COL8_Pin|COL9_Pin|COL10_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : STAT_LED1_Pin STAT_LED0_Pin */
  GPIO_InitStruct.Pin = STAT_LED1_Pin|STAT_LED0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
