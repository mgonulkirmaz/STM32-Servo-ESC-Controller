/**
 * @file main.c
 * @author Mustafa Gönülkırmaz (mgonulkrmaz@gmail.com)
 * @brief STM32F103C8 MCU based Servo - ESC Controller Board
 * @version 0.3
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
 *     Structs/Enums
 ***************************************************/

typedef enum { false, true } bool;

/**
 ****************************************************
 *     Function Prototypes
 ***************************************************/

void SystemClock_Config(void);
void PeriphClock_Config(void);
void ISR_Config(void);
void GPIO_Config(void);
void Timer_Config(void);
void Timer4_CEN(bool);
void UpdateTimerWidths(void);
void ToggleBit(bool*);

/**
 ****************************************************
 *     Defines
 ***************************************************/

#define _PSC 71             // Prescaler value
#define ARR_50Hz 19999      // ARR Value for 50Hz (20ms) at 72MHz & 71 PSC
#define ARR_200Hz 4999      // ARR Value for 200Hz (5ms) at 72MHz & 71 PSC
#define MIN_PWM_WIDTH 1000  // Minimum PWM width (microseconds)
#define MAX_PWM_WIDTH 2000  // Maximum PWM width (microseconds)
#define PWM1_PIN PB6        // PWM pin 1
#define PWM2_PIN PB7        // PWM pin 2
#define PWM3_PIN PB8        // PWM pin 3
#define PWM4_PIN PB9        // PWM pin 4
#define PWM_START_PIN PB12  // External input button to start/stop pwm

/**
 ****************************************************
 *     Variables
 ***************************************************/

uint16_t pwm1 = 1500;  // PWM Width for Channel 1
uint16_t pwm2 = 1500;  // PWM Width for Channel 2
uint16_t pwm3 = 1500;  // PWM Width for Channel 3
uint16_t pwm4 = 1500;  // PWM Width for Channel 4
bool clken = false;    // Timer state

int main() {
  SystemClock_Config();
  PeriphClock_Config();
  GPIO_Config();
  ISR_Config();
  Timer_Config();

  while (1) {
    UpdateTimerWidths();
    HAL_Delay(200);
  }
}

/**
 * @brief Error handler
 *
 */
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
      (GPIO_CRH_MODE8 | GPIO_CRH_CNF8_1 | GPIO_CRH_MODE9 | GPIO_CRH_CNF9_1 |
       (0b00 << GPIO_CRH_MODE12_Pos) | GPIO_CRH_CNF12_1);
  GPIOB->ODR |= GPIO_ODR_ODR12;
}

/**
 * @brief Interrupt configurations
 *
 */
void ISR_Config(void) {
  EXTI->FTSR |= EXTI_FTSR_FT12;
  EXTI->RTSR &= ~EXTI_RTSR_RT12;
  AFIO->EXTICR[3] = AFIO_EXTICR4_EXTI12_PB;
  EXTI->IMR |= EXTI_IMR_IM12;
  NVIC_EnableIRQ(EXTI15_10_IRQn);
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
  TIM4->CCR1 = pwm1;
  TIM4->CCR2 = pwm2;
  TIM4->CCR3 = pwm3;
  TIM4->CCR4 = pwm4;
}

/**
 * @brief Update timer pwm widths
 *
 */
void UpdateTimerWidths(void) {
  TIM4->CCR1 = pwm1;
  TIM4->CCR2 = pwm2;
  TIM4->CCR3 = pwm3;
  TIM4->CCR4 = pwm4;
}

/**
 * @brief Timer 4 Clock Enable/Disable
 *
 * @param state
 */
void Timer4_CEN(bool state) {
  if (state == true)
    TIM4->CR1 |= TIM_CR1_CEN;
  else
    TIM4->CR1 &= ~TIM_CR1_CEN;
}

/**
 * @brief Interrupt handler
 *
 */
void EXTI15_10_IRQHandler(void) {
  if (EXTI->PR & EXTI_PR_PIF12) {
    EXTI->PR |= EXTI_PR_PIF12;
  }
  ToggleBit(&clken);
  Timer4_CEN(clken);
}

/**
 * @brief Bool toggle function
 *
 * @param bit
 */
void ToggleBit(bool* bit) {
  if (*bit == false)
    *bit = true;
  else
    *bit = false;
}
