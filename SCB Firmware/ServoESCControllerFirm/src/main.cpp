/**
 * @file main.c
 * @author Mustafa Gönülkırmaz (mgonulkrmaz@gmail.com)
 * @brief STM32F103C8 MCU based Servo - ESC Controller Board
 * @version 1.0
 * @date 2021-03-31
 *
 * @copyright Copyright (c) 2021
 *
 */

/**
 ****************************************************
 *     Includes
 ***************************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <stm32f1xx.h>

/**
 ****************************************************
 *     Structs/Enums
 ***************************************************/

typedef enum {
  ROTARY_NOTHING,
  ROTARY_INCREMENT,
  ROTARY_DECRAMENT
} RotaryRotation_t;

/**
 ****************************************************
 *     Function Prototypes
 ***************************************************/

void SystemClock_Config(void);
void PeriphClock_Config(void);
void GPIO_Config(void);
void Timer_Config(void);
void Timer4_CEN(bool);
void TimerCapComSet(uint8_t, bool);
void UpdateTimerWidths(void);
void updateScreen(void);
void rotaryControl(void);
void checkRotaryRotation(void);
void onButtonPress(void);

/**
 ****************************************************
 *     Defines
 ***************************************************/

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define OLED_RESET -1  // OLED Screen defines
#define CLK_PIN PA0
#define DT_PIN PA1
#define SW_PIN PA4
#define SCL_PIN PB10        // SCL Pin
#define SDA_PIN PB11        // SDA Pin
#define PSC_VAL 71          // Prescaler value
#define ARR_50Hz 19999      // ARR Value for 50Hz (20ms) at 72MHz & 71 PSC
#define ARR_200Hz 4999      // ARR Value for 200Hz (5ms) at 72MHz & 71 PSC
#define MIN_PWM_WIDTH 500   // Minimum PWM width (microseconds)
#define MAX_PWM_WIDTH 2400  // Maximum PWM width (microseconds)
#define PWM1_PIN PB6        // PWM pin 1
#define PWM2_PIN PB7        // PWM pin 2
#define PWM3_PIN PB8        // PWM pin 3
#define PWM4_PIN PB9        // PWM pin 4

/**
 ****************************************************
 *     Variables
 ***************************************************/

uint16_t ch_value[4] = {1500, 1500, 1500, 1500};
bool ch_selected[4] = {false, false, false, false};
bool ch_en[4] = {false, false, false, false};
bool refresh = false;
uint8_t ch_counter = 0;

uint32_t lastDebounce = 0;
uint32_t debounceDelay = 10;
uint32_t lastButtonDebounce = 0;
uint32_t buttonDebounceDelay = 250;

volatile uint8_t prevPinStates;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RotaryRotation_t rotaryRotation = ROTARY_NOTHING;

void setup() {
  SystemClock_Config();
  PeriphClock_Config();
  GPIO_Config();
  Timer_Config();

  Wire.setSCL(SCL_PIN);
  Wire.setSDA(SDA_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for (;;)
      ;
  }

  prevPinStates = (GPIOA->IDR & 0x03);

  display.display();
  delay(500);
  display.clearDisplay();
  display.drawPixel(10, 10, SSD1306_WHITE);
  display.display();
  delay(250);
  display.invertDisplay(true);
  delay(500);
  display.invertDisplay(false);
  delay(500);
  display.clearDisplay();

  // For test purpose only
  for (uint8_t i = 0; i < 4; i++) {
    ch_en[i] = true;
    TimerCapComSet(i, ch_en[i]);
  }

  updateScreen();
}

void loop() {
  checkRotaryRotation();
  rotaryControl();
  rotaryRotation = ROTARY_NOTHING;

  if ((GPIOA->IDR & (1 << 4)) == 0) {
    onButtonPress();
  }
}

/**
 * @brief Rotary encoder turn check routine
 *
 */
void checkRotaryRotation(void) {
  if ((HAL_GetTick() - lastDebounce) > debounceDelay) {
    if (prevPinStates == 0x00) {
      if ((GPIOA->IDR & 0x03) == 0x01) {
        rotaryRotation = ROTARY_DECRAMENT;
      }
      if ((GPIOA->IDR & 0x03) == 0x02) {
        rotaryRotation = ROTARY_INCREMENT;
      }
    } else if (prevPinStates == 0x01) {
      if ((GPIOA->IDR & 0x03) == 0x03) {
        rotaryRotation = ROTARY_DECRAMENT;
      }
      if ((GPIOA->IDR & 0x03) == 0x00) {
        rotaryRotation = ROTARY_INCREMENT;
      }
    } else if (prevPinStates == 0x02) {
      if ((GPIOA->IDR & 0x03) == 0x00) {
        rotaryRotation = ROTARY_DECRAMENT;
      }
      if ((GPIOA->IDR & 0x03) == 0x03) {
        rotaryRotation = ROTARY_INCREMENT;
      }
    } else if (prevPinStates == 0x03) {
      if ((GPIOA->IDR & 0x03) == 0x02) {
        rotaryRotation = ROTARY_DECRAMENT;
      }
      if ((GPIOA->IDR & 0x03) == 0x01) {
        rotaryRotation = ROTARY_INCREMENT;
      }
    }
    prevPinStates = (GPIOA->IDR & 0x03);
    lastDebounce = HAL_GetTick();
  }
}

/**
 * @brief Rotary button check routine
 *
 */
void onButtonPress(void) {
  if ((millis() - lastButtonDebounce) > buttonDebounceDelay) {
    switch (ch_counter) {
      case 0:
        ch_selected[0] = !ch_selected[0];
        break;
      case 1:
        ch_selected[1] = !ch_selected[1];
        break;
      case 2:
        ch_selected[2] = !ch_selected[2];
        break;
      case 3:
        ch_selected[3] = !ch_selected[3];
        break;
    }
    UpdateTimerWidths();
    updateScreen();
    lastButtonDebounce = HAL_GetTick();
  }
}

/**
 * @brief Rotary encoder control routine
 *
 */
void rotaryControl(void) {
  if (ch_selected[0] == true) {
    if (rotaryRotation != ROTARY_NOTHING) {
      if (rotaryRotation == ROTARY_INCREMENT) {
        if (ch_value[0] < MAX_PWM_WIDTH - 10) {
          ch_value[0] += 10;
        } else {
          ch_value[0] = MAX_PWM_WIDTH;
        }
      } else if (rotaryRotation == ROTARY_DECRAMENT) {
        if (ch_value[0] < MIN_PWM_WIDTH + 10) {
          ch_value[0] = MIN_PWM_WIDTH;
        } else {
          ch_value[0] -= 10;
        }
      }
    }
  } else if (ch_selected[1] == true) {
    if (rotaryRotation != ROTARY_NOTHING) {
      if (rotaryRotation == ROTARY_INCREMENT) {
        if (ch_value[1] < MAX_PWM_WIDTH - 10) {
          ch_value[1] += 10;
        } else {
          ch_value[1] = MAX_PWM_WIDTH;
        }
      } else if (rotaryRotation == ROTARY_DECRAMENT) {
        if (ch_value[1] < MIN_PWM_WIDTH + 10) {
          ch_value[1] = MIN_PWM_WIDTH;
        } else {
          ch_value[1] -= 10;
        }
      }
    }
  } else if (ch_selected[2] == true) {
    if (rotaryRotation != ROTARY_NOTHING) {
      if (rotaryRotation == ROTARY_INCREMENT) {
        if (ch_value[2] < MAX_PWM_WIDTH - 10) {
          ch_value[2] += 10;
        } else {
          ch_value[2] = MAX_PWM_WIDTH;
        }
      } else if (rotaryRotation == ROTARY_DECRAMENT) {
        if (ch_value[2] < MIN_PWM_WIDTH + 10) {
          ch_value[2] = MIN_PWM_WIDTH;
        } else {
          ch_value[2] -= 10;
        }
      }
    }
  } else if (ch_selected[3] == true) {
    if (rotaryRotation != ROTARY_NOTHING) {
      if (rotaryRotation == ROTARY_INCREMENT) {
        if (ch_value[3] < MAX_PWM_WIDTH - 10) {
          ch_value[3] += 10;
        } else {
          ch_value[3] = MAX_PWM_WIDTH;
        }
      } else if (rotaryRotation == ROTARY_DECRAMENT) {
        if (ch_value[3] < MIN_PWM_WIDTH + 10) {
          ch_value[3] = MIN_PWM_WIDTH;
        } else {
          ch_value[3] -= 10;
        }
      }
    }
  } else {
    if (rotaryRotation != ROTARY_NOTHING) {
      if (rotaryRotation == ROTARY_INCREMENT) {
        if (ch_counter < 3) {
          ch_counter++;
        } else {
          ch_counter = 0;
        }
      } else if (rotaryRotation == ROTARY_DECRAMENT) {
        if (ch_counter < 1) {
          ch_counter = 3;
        } else {
          ch_counter--;
        }
      }
    }
  }
  if (rotaryRotation != ROTARY_NOTHING) {
    updateScreen();
  }
}

/**
 * @brief Update screen routine
 *
 */
void updateScreen(void) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("KANAL 1: ");
  display.setCursor(60, 10);
  display.println(ch_value[0]);
  display.setCursor(100, 10);
  display.println(ch_en[0]);

  display.setCursor(10, 20);
  display.println("KANAL 2: ");
  display.setCursor(60, 20);
  display.println(ch_value[1]);
  display.setCursor(100, 20);
  display.println(ch_en[1]);

  display.setCursor(10, 30);
  display.println("KANAL 3: ");
  display.setCursor(60, 30);
  display.println(ch_value[2]);
  display.setCursor(100, 30);
  display.println(ch_en[2]);

  display.setCursor(10, 40);
  display.println("KANAL 4: ");
  display.setCursor(60, 40);
  display.println(ch_value[3]);
  display.setCursor(100, 40);
  display.println(ch_en[3]);

  if (ch_selected[0] == true) {
    display.setCursor(2, (ch_counter * 10) + 10);
    display.print("X");
  } else if (ch_selected[1] == true) {
    display.setCursor(2, (ch_counter * 10) + 10);
    display.print("X");
  } else if (ch_selected[2] == true) {
    display.setCursor(2, (ch_counter * 10) + 10);
    display.print("X");
  } else if (ch_selected[3] == true) {
    display.setCursor(2, (ch_counter * 10) + 10);
    display.print("X");
  } else {
    display.setCursor(2, (ch_counter * 10) + 10);
    display.println(">");
  }

  display.display();
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

  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
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
  TIM4->CCER = 0;
  TIM4->PSC = PSC_VAL;
  TIM4->ARR = ARR_50Hz;
  TIM4->DCR = 0;
  TIM4->CCR1 = ch_value[0];
  TIM4->CCR2 = ch_value[1];
  TIM4->CCR3 = ch_value[2];
  TIM4->CCR4 = ch_value[3];
  TIM4->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Update timer pwm widths
 *
 */
void UpdateTimerWidths(void) {
  TIM4->CCR1 = ch_value[0];
  TIM4->CCR2 = ch_value[1];
  TIM4->CCR3 = ch_value[2];
  TIM4->CCR4 = ch_value[3];
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
 * @brief Timer Capture/Compare Enable/Disable function
 *
 */
void TimerCapComSet(uint8_t channel, bool setValue) {
  if (channel >= 0 && channel < 4) {
    if (setValue) {
      TIM4->CCER |= (1 << channel);
    } else {
      TIM4->CCER &= ~(1 << channel);
    }
  }
}