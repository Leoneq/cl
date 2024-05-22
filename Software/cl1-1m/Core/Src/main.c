/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
 *
 * Do To:
 * kompensacaj oscylatora dla temperatury
 * kalibracja oscylatora
 *
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <math.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RTC_ADDRESS 0xD0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim14;

/* USER CODE BEGIN PV */

static GPIO_TypeDef *const DIG_Port[] = { D0_GPIO_Port, D1_GPIO_Port,
D2_GPIO_Port, D3_GPIO_Port };

static const uint16_t DIG_Pin[] = { D0_Pin, D1_Pin, D2_Pin, D3_Pin };

static const uint16_t gpio_pins[] = { 4, 0, 1, 4, 2 };

RTC_TimeTypeDef time;
RTC_DateTypeDef date;

volatile uint8_t isPressed = 3; // Zmienna do zapamiętania który przycisk zosta?��? wciśnięty, jeżeli jest równa 3 to nic nie było wciśnięte
volatile uint32_t actTick = 0; // Zmienna do zapamiętania czasu wciśnięcia przycisku
volatile int dispValue = 0; // Wyświetlana liczba
volatile int curDig = 0; // Wyświetlana Cyfra
volatile uint8_t selDig = 0; // Wybrana Cyfra
uint8_t clickType = 0; // od 1 do 3 to krótkie przyciśnięcia, od 4 do 6 długie przyciśnięcia
uint8_t setupMode = 0; // Jeżeli jest większe od zera to zegarek jest w trybie programowania
uint8_t setupDig = 0;
uint8_t newDig = 0;
uint16_t pwm = 50;
uint16_t brightness = 0;
uint16_t onBright = 50;
uint16_t offBright = 5;
uint16_t onTime = 0000;
uint16_t offTime = 0000;

// Ustawianie segmentów
const uint8_t digits[15] = { 0b00111111, // 0
		0b00000110, // 1
		0b01011011, // 2
		0b01001111, // 3
		0b01100110, // 4
		0b01101101, // 5
		0b01111101, // 6
		0b00000111, // 7
		0b01111111, // 8
		0b01101111, // 9
		0b00000000, // Blank
		0b01111001, 0b01010000, 0b01010000, 0b00000000 };
static GPIO_TypeDef *const SEG_Port[] = { SA_GPIO_Port, SB_GPIO_Port,
SC_GPIO_Port, SD_GPIO_Port, SE_GPIO_Port, SF_GPIO_Port, SG_GPIO_Port,
SDP_GPIO_Port };

static const uint16_t SEG_Pin[] = { SA_Pin, SB_Pin, SC_Pin, SD_Pin, SE_Pin,
SF_Pin, SG_Pin, SDP_Pin };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM14_Init(void);
static void MX_RTC_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void setDigit(int digit) {
	for (int i = 0; i < 8; i++) {
		HAL_GPIO_WritePin(SEG_Port[i], SEG_Pin[i],
				((((digits[digit] >> i) & 1) == 1) ?
						GPIO_PIN_SET : GPIO_PIN_RESET));

	}
}

void customTick() {
	if (isPressed != 3)
		actTick++;
}

// Obsługa przerwa?��? wywoływanych przez przyciski
void swToggle(uint16_t GPIO_Pin) {
	if ((HAL_GPIO_ReadPin(GPIOD, GPIO_Pin) == GPIO_PIN_RESET)
			&& (isPressed == 3)) {
		actTick = 0;
		isPressed = GPIO_Pin;
	} else if ((HAL_GPIO_ReadPin(GPIOD, GPIO_Pin) == GPIO_PIN_SET)
			&& (isPressed == GPIO_Pin)) {
		if (actTick > 50 && actTick < 700) {
			clickType = gpio_pins[GPIO_Pin] + 1;
		} else if (actTick > 50) {
			clickType = gpio_pins[GPIO_Pin] + 4;
		}
		actTick = 0;
		isPressed = 3;
	} else {
		__NOP();
	}
}
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
	swToggle(GPIO_Pin);
}
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
	swToggle(GPIO_Pin);
}
volatile long delayForBlink = 0;
volatile long delay = 0;
// Przerwanie do odświeżania zegarka

void TIM14_Callback() {
	if (delay == 0) {
		HAL_GPIO_WritePin(DIG_Port[(curDig + 3) % 4], DIG_Pin[(curDig + 3) % 4],
				GPIO_PIN_RESET);
		setDigit(10);
	} else if (delay == 5) {
		HAL_GPIO_WritePin(DIG_Port[(curDig + 3) % 4], DIG_Pin[(curDig + 3) % 4],
				GPIO_PIN_RESET);
		if (HAL_I2C_IsDeviceReady(&hi2c1,RTC_ADDRESS, 3,5) != HAL_OK) {
			setDigit(14 -  curDig);
		} else if (setupDig == curDig
				&& (setupMode == 1 || setupMode == 3 || setupMode == 4)) {
			setDigit(
					(delayForBlink < 50000) ?
							((int) (dispValue / pow(10.0, curDig)) % 10) : 10);
		} else if (setupDig == 0 && (curDig == 0 || curDig == 1)
				&& setupMode == 2) {
			setDigit(
					(delayForBlink < 50000) ?
							((int) (dispValue / pow(10.0, curDig)) % 10) : 10);
		} else if (setupDig == 1 && (curDig == 2 || curDig == 3)
				&& setupMode == 2) {
			setDigit(
					(delayForBlink < 50000) ?
							((int) (dispValue / pow(10.0, curDig)) % 10) : 10);
		} else {
			setDigit((int) (dispValue / pow(10.0, curDig)) % 10);
		}
		HAL_GPIO_WritePin(SEG_Port[7], SEG_Pin[7],
				((setupMode - 1) == curDig) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	} else if (setupMode == 2) {
		if (curDig == 0 || curDig == 1) {
			if (delay == 126 - onBright) {
				HAL_GPIO_WritePin(DIG_Port[curDig], DIG_Pin[curDig],
						GPIO_PIN_SET);
			}
		} else {
			if (delay == 126 - offBright) {
				HAL_GPIO_WritePin(DIG_Port[curDig], DIG_Pin[curDig],
						GPIO_PIN_SET);
			}

		}
	} else if (delay >= (126 - brightness)) {
		HAL_GPIO_WritePin(DIG_Port[curDig], DIG_Pin[curDig], GPIO_PIN_SET);

	}

	if (delay == 125) {
		delay = 0;
		curDig = (curDig + 1) % 4;
	} else {
		delay++;
	}
	if (delayForBlink == 100000) {
		delayForBlink = 0;
	} else {
		delayForBlink++;
	}
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

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
	MX_ADC1_Init();
	MX_TIM3_Init();
	MX_TIM14_Init();
	MX_RTC_Init();
	MX_I2C1_Init();
	/* USER CODE BEGIN 2 */
	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_TIM_Base_Start_IT(&htim14);
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

	onTime = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
	offTime = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
	onBright =
			(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2) != 0) ?
					HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2) : 50;
	offBright = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

// Obsługa przetwornicy
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		uint32_t value = HAL_ADC_GetValue(&hadc1);
		float voltage = (((3.3 * value)) / 4096) * 33.6;
		if (40 < voltage) {
			if (pwm > 0)
				pwm--;
		} else {
			if (pwm < 198)
				pwm++;
		}
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm);
		if (setupMode == 0) { // Dla setupMode równego 0 zegarek pracuje normalnie

			if (dispValue % 100 != time.Minutes) {
				dispValue = (time.Hours * 100) + time.Minutes;
			}
			if (onTime < offTime) {
				if (dispValue >= onTime && dispValue < offTime) {
					brightness = onBright;
				} else {
					brightness = offBright;
				}

			} else if (onTime > offTime) {
				if (dispValue < onTime && dispValue >= offTime) {
					brightness = offBright;
				} else {
					brightness = onBright;
				}
			} else {
				brightness = onBright;
			}
			if (clickType == 5) {
				clickType = 0;
				setupDig = 0;
				setupMode++;
			}
		} else if (setupMode != 5) {
			brightness = 50;
			if (clickType == 5) {
				clickType = 0;
				setupMode = 0;
			} else if (clickType == 4) {
				clickType = 0;
				setupDig = 0;
				setupMode = (setupMode == 1) ? 4 : setupMode - 1;
			} else if (clickType == 6) {
				clickType = 0;
				setupDig = 0;
				setupMode = (setupMode >= 4) ? 1 : setupMode + 1;
			}
			switch (setupMode) {
			case 1:
				dispValue = (time.Hours * 100) + time.Minutes;
				newDig = ((int) (dispValue / pow(10.0, setupDig)) % 10);
				if (clickType == 2) {
					delayForBlink = 25000;
					clickType = 0;
					setupDig = (setupDig + 1) % 4;
					newDig = ((int) (dispValue / pow(10.0, setupDig)) % 10);
				} else if (clickType == 1) {
					clickType = 0;
					if (setupDig == 0) {
						newDig = (newDig + 1) % 10;
					} else if (setupDig == 1) {
						newDig = (newDig + 1) % 6;
					} else if (setupDig == 2) {
						if (((dispValue / 1000) % 10) == 2) {
							newDig = (newDig + 1) % 5;
						} else {
							newDig = (newDig + 1) % 10;
						}
					} else if (setupDig == 3) {
						if (((dispValue / 100) % 10) > 4) {
							newDig = (newDig + 1) % 2;
						} else {
							newDig = (newDig + 1) % 3;
						}
					}
				} else if (clickType == 3) {
					clickType = 0;
					if (setupDig == 0) {
						newDig = (newDig + 9) % 10;
					} else if (setupDig == 1) {
						newDig = (newDig + 5) % 6;
					} else if (setupDig == 2) {
						if (((dispValue / 1000) % 10) == 2) {
							newDig = (newDig + 4) % 5;
						} else {
							newDig = (newDig + 9) % 10;
						}
					} else if (setupDig == 3) {
						if (((dispValue / 100) % 10) > 4) {
							newDig = (newDig + 1) % 2;
						} else {
							newDig = (newDig + 2) % 3;
						}
					}
				}
				if (newDig != ((int) (dispValue / pow(10.0, setupDig)) % 10)) {
					delayForBlink = 0;
					dispValue = (dispValue
							- (((int) (dispValue / pow(10.0, setupDig)) % 10))
									* pow(10.0, setupDig))
							+ (newDig * pow(10.0, setupDig));
					time.Hours = dispValue / 100;
					time.Minutes = dispValue % 100;
					time.Seconds = 0;
					//HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
				}

				break;
			case 2:
				dispValue = ((onBright == 100) ? 99 : onBright)
						+ (((offBright == 100) ? 99 : offBright) * 100);
				if (clickType == 2) {
					delayForBlink = 25000;
					clickType = 0;
					setupDig = (setupDig + 1) % 2;

				} else if (clickType == 1) {
					delayForBlink = 0;
					clickType = 0;
					if (setupDig == 0) {
						onBright = (((onBright / 5) + 1) % 21) * 5;
						HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, onBright);
					} else {
						offBright = (((offBright / 5) + 1) % 21) * 5;
						HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, offBright);
					}

				} else if (clickType == 3) {
					delayForBlink = 0;
					clickType = 0;
					if (setupDig == 0) {
						onBright = (((onBright / 5) + 20) % 21) * 5;
						HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, onBright);
					} else {
						HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, offBright);
						offBright = (((offBright / 5) + 20) % 21) * 5;
					}
				}
				break;
			case 3:
				dispValue = onTime;
				newDig = ((int) (onTime / pow(10.0, setupDig)) % 10);
				if (clickType == 2) {
					delayForBlink = 25000;
					clickType = 0;
					setupDig = (setupDig + 1) % 4;
					newDig = ((int) (onTime / pow(10.0, setupDig)) % 10);
				} else if (clickType == 1) {
					clickType = 0;
					if (setupDig == 0) {
						newDig = (newDig + 1) % 10;
					} else if (setupDig == 1) {
						newDig = (newDig + 1) % 6;
					} else if (setupDig == 2) {
						if (((onTime / 1000) % 10) == 2) {
							newDig = (newDig + 1) % 5;
						} else {
							newDig = (newDig + 1) % 10;
						}
					} else if (setupDig == 3) {
						if (((onTime / 100) % 10) > 4) {
							newDig = (newDig + 1) % 2;
						} else {
							newDig = (newDig + 1) % 3;
						}
					}
				} else if (clickType == 3) {
					clickType = 0;
					if (setupDig == 0) {
						newDig = (newDig + 9) % 10;
					} else if (setupDig == 1) {
						newDig = (newDig + 5) % 6;
					} else if (setupDig == 2) {
						if (((onTime / 1000) % 10) == 2) {
							newDig = (newDig + 4) % 5;
						} else {
							newDig = (newDig + 9) % 10;
						}
					} else if (setupDig == 3) {
						if (((onTime / 100) % 10) > 4) {
							newDig = (newDig + 1) % 2;
						} else {
							newDig = (newDig + 2) % 3;
						}
					}
				}
				if (newDig != ((int) (onTime / pow(10.0, setupDig)) % 10)) {
					delayForBlink = 0;
					onTime = (onTime
							- (((int) (onTime / pow(10.0, setupDig)) % 10))
									* pow(10.0, setupDig))
							+ (newDig * pow(10.0, setupDig));
					HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, onTime);
				}

				break;
			case 4:
				dispValue = offTime;
				newDig = ((int) (offTime / pow(10.0, setupDig)) % 10);
				if (clickType == 2) {
					delayForBlink = 25000;
					clickType = 0;
					setupDig = (setupDig + 1) % 4;
					newDig = ((int) (offTime / pow(10.0, setupDig)) % 10);
				} else if (clickType == 1) {
					clickType = 0;
					if (setupDig == 0) {
						newDig = (newDig + 1) % 10;
					} else if (setupDig == 1) {
						newDig = (newDig + 1) % 6;
					} else if (setupDig == 2) {
						if (((offTime / 1000) % 10) == 2) {
							newDig = (newDig + 1) % 5;
						} else {
							newDig = (newDig + 1) % 10;
						}
					} else if (setupDig == 3) {
						if (((offTime / 100) % 10) > 4) {
							newDig = (newDig + 1) % 2;
						} else {
							newDig = (newDig + 1) % 3;
						}
					}
				} else if (clickType == 3) {
					clickType = 0;
					if (setupDig == 0) {
						newDig = (newDig + 9) % 10;
					} else if (setupDig == 1) {
						newDig = (newDig + 5) % 6;
					} else if (setupDig == 2) {
						if (((offTime / 1000) % 10) == 2) {
							newDig = (newDig + 4) % 5;
						} else {
							newDig = (newDig + 9) % 10;
						}
					} else if (setupDig == 3) {
						if (((offTime / 100) % 10) > 4) {
							newDig = (newDig + 1) % 2;
						} else {
							newDig = (newDig + 2) % 3;
						}
					}
				}
				if (newDig != ((int) (offTime / pow(10.0, setupDig)) % 10)) {
					delayForBlink = 0;
					offTime = (offTime
							- (((int) (offTime / pow(10.0, setupDig)) % 10))
									* pow(10.0, setupDig))
							+ (newDig * pow(10.0, setupDig));
					HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, offTime);
				}

				break;

			}
		}
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
	RCC_OscInitStruct.PLL.PLLN = 8;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig = { 0 };

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */

	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.LowPowerAutoPowerOff = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_1CYCLE_5;
	hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
	hadc1.Init.OversamplingMode = DISABLE;
	hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}

	/** Configure Regular Channel
	 */
	sConfig.Channel = ADC_CHANNEL_8;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */

}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.Timing = 0x00300F38;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}

	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE)
			!= HAL_OK) {
		Error_Handler();
	}

	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void) {

	/* USER CODE BEGIN RTC_Init 0 */

	/* USER CODE END RTC_Init 0 */

	/* USER CODE BEGIN RTC_Init 1 */

	/* USER CODE END RTC_Init 1 */

	/** Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 127;
	hrtc.Init.SynchPrediv = 255;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN RTC_Init 2 */

	/* USER CODE END RTC_Init 2 */

}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void) {

	/* USER CODE BEGIN TIM3_Init 0 */

	/* USER CODE END TIM3_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };

	/* USER CODE BEGIN TIM3_Init 1 */

	/* USER CODE END TIM3_Init 1 */
	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 427;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM3_Init 2 */

	/* USER CODE END TIM3_Init 2 */
	HAL_TIM_MspPostInit(&htim3);

}

/**
 * @brief TIM14 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM14_Init(void) {

	/* USER CODE BEGIN TIM14_Init 0 */

	/* USER CODE END TIM14_Init 0 */

	/* USER CODE BEGIN TIM14_Init 1 */

	/* USER CODE END TIM14_Init 1 */
	htim14.Instance = TIM14;
	htim14.Init.Prescaler = 31;
	htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim14.Init.Period = 9;
	htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim14) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM14_Init 2 */

	/* USER CODE END TIM14_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA,
			SA_Pin | SB_Pin | SC_Pin | SD_Pin | SE_Pin | SF_Pin | SG_Pin
					| SDP_Pin | D0_Pin | D1_Pin | D2_Pin | D3_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pins : SA_Pin SB_Pin D0_Pin D1_Pin */
	GPIO_InitStruct.Pin = SA_Pin | SB_Pin | D0_Pin | D1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : SC_Pin SD_Pin SE_Pin SF_Pin
	 SG_Pin SDP_Pin D2_Pin D3_Pin */
	GPIO_InitStruct.Pin = SC_Pin | SD_Pin | SE_Pin | SF_Pin | SG_Pin | SDP_Pin
			| D2_Pin | D3_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : SW0_Pin SW1_Pin SW2_Pin */
	GPIO_InitStruct.Pin = SW0_Pin | SW1_Pin | SW2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/**/
	__HAL_SYSCFG_FASTMODEPLUS_ENABLE(SYSCFG_FASTMODEPLUS_PA9);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

	HAL_NVIC_SetPriority(EXTI2_3_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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
