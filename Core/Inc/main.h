/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdbool.h>
#include <stdint.h>

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/**
 * Wait for us microseconds
 *
 * @param us the number of microseconds to wait
 */
extern void delayMicroseconds(uint16_t us);

/**
 * Set the status LEDs in response to the set LED command (0xed)
 *
 * @param status the bitfield for all the status LEDs
 */
extern void ps2_set_leds(uint8_t status);

/**
 * Return the number of milliseconds since startup
 */
extern long millis();

/**
 * Set whether or not keyboard scanning is enabled
 *
 * @param enable true if the keyboard scanning should be enabled, false otherwise
 */
extern void kbd_set_enable(bool enable);

/**
 * Set the initial delay and delay rate in milliseconds for the key repeat (typematic) function
 *
 * @param delay_ms the initial delay in milliseconds
 * @param rate_ms the repeat rate delay in milliseconds
 */
extern void kbd_set_typematic(short delay_ms, short rate_ms);

/**
 * Set whether or not an LED is on
 *
 * @param led the index of the LED to update (0 -- 11)
 * @param is_on TRUE if the LED should be on, FALSE if it should be off
 */
extern void ws2812_set_state(short led, bool is_on);

/**
 * Set the color of a given LED in the records, but do not actually update the LEDs
 *
 * @param led the index of the LED to update (0 -- 11)
 * @param red the red component 0 -- 255
 * @param green the green component 0 -- 255
 * @param blue the blue component 0 -- 255
 */
extern void ws2812_set_color(unsigned short led, uint8_t red, uint8_t green, uint8_t blue);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ROW1_Pin GPIO_PIN_0
#define ROW1_GPIO_Port GPIOC
#define ROW2_Pin GPIO_PIN_1
#define ROW2_GPIO_Port GPIOC
#define ROW3_Pin GPIO_PIN_2
#define ROW3_GPIO_Port GPIOC
#define ROW4_Pin GPIO_PIN_3
#define ROW4_GPIO_Port GPIOC
#define PS2_CLK_IN_Pin GPIO_PIN_0
#define PS2_CLK_IN_GPIO_Port GPIOA
#define PS2_CLK_OUT_Pin GPIO_PIN_1
#define PS2_CLK_OUT_GPIO_Port GPIOA
#define PS2_DATA_IN_Pin GPIO_PIN_2
#define PS2_DATA_IN_GPIO_Port GPIOA
#define PS2_DATA_OUT_Pin GPIO_PIN_3
#define PS2_DATA_OUT_GPIO_Port GPIOA
#define ROW5_Pin GPIO_PIN_4
#define ROW5_GPIO_Port GPIOC
#define ROW6_Pin GPIO_PIN_5
#define ROW6_GPIO_Port GPIOC
#define COL1_Pin GPIO_PIN_0
#define COL1_GPIO_Port GPIOB
#define COL2_Pin GPIO_PIN_1
#define COL2_GPIO_Port GPIOB
#define COL3_Pin GPIO_PIN_2
#define COL3_GPIO_Port GPIOB
#define COL11_Pin GPIO_PIN_10
#define COL11_GPIO_Port GPIOB
#define COL12_Pin GPIO_PIN_11
#define COL12_GPIO_Port GPIOB
#define COL13_Pin GPIO_PIN_12
#define COL13_GPIO_Port GPIOB
#define COL14_Pin GPIO_PIN_13
#define COL14_GPIO_Port GPIOB
#define COL15_Pin GPIO_PIN_14
#define COL15_GPIO_Port GPIOB
#define COL16_Pin GPIO_PIN_15
#define COL16_GPIO_Port GPIOB
#define STAT_LED1_Pin GPIO_PIN_11
#define STAT_LED1_GPIO_Port GPIOA
#define STAT_LED0_Pin GPIO_PIN_12
#define STAT_LED0_GPIO_Port GPIOA
#define COL4_Pin GPIO_PIN_3
#define COL4_GPIO_Port GPIOB
#define COL5_Pin GPIO_PIN_4
#define COL5_GPIO_Port GPIOB
#define COL6_Pin GPIO_PIN_5
#define COL6_GPIO_Port GPIOB
#define COL7_Pin GPIO_PIN_6
#define COL7_GPIO_Port GPIOB
#define COL8_Pin GPIO_PIN_7
#define COL8_GPIO_Port GPIOB
#define COL9_Pin GPIO_PIN_8
#define COL9_GPIO_Port GPIOB
#define COL10_Pin GPIO_PIN_9
#define COL10_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
