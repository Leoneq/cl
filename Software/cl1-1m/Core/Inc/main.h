/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32g0xx_hal.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SA_Pin GPIO_PIN_0
#define SA_GPIO_Port GPIOA
#define SB_Pin GPIO_PIN_1
#define SB_GPIO_Port GPIOA
#define SC_Pin GPIO_PIN_2
#define SC_GPIO_Port GPIOA
#define SD_Pin GPIO_PIN_3
#define SD_GPIO_Port GPIOA
#define SE_Pin GPIO_PIN_4
#define SE_GPIO_Port GPIOA
#define SF_Pin GPIO_PIN_5
#define SF_GPIO_Port GPIOA
#define SG_Pin GPIO_PIN_6
#define SG_GPIO_Port GPIOA
#define SDP_Pin GPIO_PIN_7
#define SDP_GPIO_Port GPIOA
#define D0_Pin GPIO_PIN_8
#define D0_GPIO_Port GPIOA
#define D1_Pin GPIO_PIN_9
#define D1_GPIO_Port GPIOA
#define D2_Pin GPIO_PIN_10
#define D2_GPIO_Port GPIOA
#define D3_Pin GPIO_PIN_11
#define D3_GPIO_Port GPIOA
#define SW0_Pin GPIO_PIN_0
#define SW0_GPIO_Port GPIOD
#define SW0_EXTI_IRQn EXTI0_1_IRQn
#define SW1_Pin GPIO_PIN_1
#define SW1_GPIO_Port GPIOD
#define SW1_EXTI_IRQn EXTI0_1_IRQn
#define SW2_Pin GPIO_PIN_2
#define SW2_GPIO_Port GPIOD
#define SW2_EXTI_IRQn EXTI2_3_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
