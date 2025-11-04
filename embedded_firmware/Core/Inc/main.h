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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_2
#define LED_GPIO_Port GPIOE
#define Z2_LIMIT_Pin GPIO_PIN_3
#define Z2_LIMIT_GPIO_Port GPIOE
#define Vacuum_3_Pin GPIO_PIN_4
#define Vacuum_3_GPIO_Port GPIOE
#define Z2_EN_Pin GPIO_PIN_5
#define Z2_EN_GPIO_Port GPIOE
#define VALVE_10_Pin GPIO_PIN_6
#define VALVE_10_GPIO_Port GPIOE
#define Vacuum_2_Pin GPIO_PIN_1
#define Vacuum_2_GPIO_Port GPIOA
#define Vacuum_1_Pin GPIO_PIN_2
#define Vacuum_1_GPIO_Port GPIOA
#define buzzer_Pin GPIO_PIN_15
#define buzzer_GPIO_Port GPIOB
#define VALVE_1_Pin GPIO_PIN_9
#define VALVE_1_GPIO_Port GPIOD
#define CTRL_8_Pin GPIO_PIN_10
#define CTRL_8_GPIO_Port GPIOD
#define VALVE_2_Pin GPIO_PIN_11
#define VALVE_2_GPIO_Port GPIOD
#define CTRL7_Pin GPIO_PIN_12
#define CTRL7_GPIO_Port GPIOD
#define VALVE_3_Pin GPIO_PIN_13
#define VALVE_3_GPIO_Port GPIOD
#define CTRL6_Pin GPIO_PIN_14
#define CTRL6_GPIO_Port GPIOD
#define VALVE_4_Pin GPIO_PIN_15
#define VALVE_4_GPIO_Port GPIOD
#define CTRL_5_Pin GPIO_PIN_2
#define CTRL_5_GPIO_Port GPIOG
#define VALVE_5_Pin GPIO_PIN_3
#define VALVE_5_GPIO_Port GPIOG
#define SOLENOID_4_Pin GPIO_PIN_4
#define SOLENOID_4_GPIO_Port GPIOG
#define VALVE_6_Pin GPIO_PIN_5
#define VALVE_6_GPIO_Port GPIOG
#define SOLENOID_3_Pin GPIO_PIN_6
#define SOLENOID_3_GPIO_Port GPIOG
#define VALVE_7_Pin GPIO_PIN_7
#define VALVE_7_GPIO_Port GPIOG
#define SOLENOID_2_Pin GPIO_PIN_8
#define SOLENOID_2_GPIO_Port GPIOG
#define VALVE_8_Pin GPIO_PIN_6
#define VALVE_8_GPIO_Port GPIOC
#define SOLENOID_1_Pin GPIO_PIN_7
#define SOLENOID_1_GPIO_Port GPIOC
#define VALVE_9_Pin GPIO_PIN_8
#define VALVE_9_GPIO_Port GPIOC
#define Vacuum_4_Pin GPIO_PIN_8
#define Vacuum_4_GPIO_Port GPIOA
#define VALVE_11_Pin GPIO_PIN_15
#define VALVE_11_GPIO_Port GPIOA
#define VALVE_12_Pin GPIO_PIN_10
#define VALVE_12_GPIO_Port GPIOC
#define X_STEP_Pin GPIO_PIN_12
#define X_STEP_GPIO_Port GPIOC
#define X_DIR_Pin GPIO_PIN_1
#define X_DIR_GPIO_Port GPIOD
#define X_EN_Pin GPIO_PIN_3
#define X_EN_GPIO_Port GPIOD
#define X_LIMIT_Pin GPIO_PIN_7
#define X_LIMIT_GPIO_Port GPIOD
#define Y_STEP_Pin GPIO_PIN_10
#define Y_STEP_GPIO_Port GPIOG
#define Y_DIR_Pin GPIO_PIN_12
#define Y_DIR_GPIO_Port GPIOG
#define Y_EN_Pin GPIO_PIN_14
#define Y_EN_GPIO_Port GPIOG
#define Y_LIMIT_Pin GPIO_PIN_15
#define Y_LIMIT_GPIO_Port GPIOG
#define Z1_STEP_Pin GPIO_PIN_3
#define Z1_STEP_GPIO_Port GPIOB
#define Z1_DIR_Pin GPIO_PIN_5
#define Z1_DIR_GPIO_Port GPIOB
#define Z1_ENA_Pin GPIO_PIN_9
#define Z1_ENA_GPIO_Port GPIOB
#define Z1_LIMIT_Pin GPIO_PIN_1
#define Z1_LIMIT_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
