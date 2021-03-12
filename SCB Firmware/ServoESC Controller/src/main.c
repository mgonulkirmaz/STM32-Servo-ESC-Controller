/**
 * @file main.c
 * @author Mustafa Gönülkırmaz (mgonulkrmaz@gmail.com)
 * @brief STM32F103C8 MCU based Servo - ESC Controller Board
 * @version 0.1
 * @date 2021-03-12
 *
 * @copyright Copyright (c) 2021
 *
 */

/**
 ****************************************************
 *     Includes
 ***************************************************/
#include <stdint.h>
#include <stm32f1xx.h>

/**
 ****************************************************
 *     Function Prototypes
 ***************************************************/
void SystemClock_Config(void);
void PeriphClock_Config(void);
void GPIO_Config(void);
void Timer_Config(void);

/**
 ****************************************************
 *     Defines
 ***************************************************/
#define _PSC 71             // Prescaler value
#define ARR_50Hz 19999      // ARR Value for 50Hz (20ms) at 72MHz & 71 PSC
#define ARR_200Hz 4999      // ARR Value for 200Hz (5ms) at 72MHz & 71 PSC
#define MIN_PWM_WIDTH 1000  // Minimum PWM witdh (microseconds)
#define MAX_PWM_WIDTH 2000  // Maximum PWM witdh (microseconds)

int main() {
  SystemClock_Config();
  PeriphClock_Config();
  GPIO_Config();
  Timer_Config();

  while (1) {
    TIM4->CR1 |= TIM_CR1_CEN;
    HAL_Delay(100);
    TIM4->CR1 &= ~(TIM_CR1_CEN);
    TIM4->CNT = 0;
    HAL_Delay(400);
  }
}

void Error_Handler(void) {
  __disable_irq();
  while (1) {
  }
}

/**
 * @brief System clock configurations
 *
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief Periphals clock configurations
 *
 */
void PeriphClock_Config(void) {
  RCC->APB2ENR = RCC_APB2ENR_IOPBEN;
  RCC->APB1ENR = RCC_APB1ENR_TIM4EN;
}

/**
 * @brief GPIO configurations
 *
 */
void GPIO_Config(void) {
  GPIOB->CRL =
      (GPIO_CRL_MODE6 | GPIO_CRL_CNF6_1 | GPIO_CRL_MODE7 | GPIO_CRL_CNF7_1);
  GPIOB->CRH =
      (GPIO_CRH_MODE8 | GPIO_CRH_CNF8_1 | GPIO_CRH_MODE9 | GPIO_CRH_CNF9_1);
}

/**
 * @brief Timer configurations
 *
 */
void Timer_Config(void) {
  TIM4->CR1 = TIM_CR1_ARPE;
  TIM4->CR2 = 0;
  TIM4->SMCR = 0;
  TIM4->DIER = 0;
  TIM4->EGR = 0;
  TIM4->CCMR1 = (0b110 << TIM_CCMR1_OC1M_Pos | TIM_CCMR1_OC1PE |
                 0b110 << TIM_CCMR1_OC2M_Pos | TIM_CCMR1_OC2PE);
  TIM4->CCMR2 = (0b110 << TIM_CCMR2_OC3M_Pos | TIM_CCMR2_OC3PE |
                 0b110 << TIM_CCMR2_OC4M_Pos | TIM_CCMR2_OC4PE);
  TIM4->CCER = (TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E);
  TIM4->PSC = _PSC;
  TIM4->ARR = ARR_50Hz;
  TIM4->DCR = 0;
  TIM4->CCR1 = 1000;
  TIM4->CCR2 = 1300;
  TIM4->CCR3 = 1600;
  TIM4->CCR4 = 2000;
}
