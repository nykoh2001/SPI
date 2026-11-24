/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "gpio.h"
#include "stm32f746xx.h"
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  /* USER CODE BEGIN 2 */

  /*
  * PB5: SPI1_MOSI
  * PA6: SPI1_MISO
  * PA5: SPI1_SCK
  */
  // Step1: clock (RM p. 71~72)
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  // Step2: Alternate (RM p.218, p.211)
  // PB5, MOSI
  GPIOB->MODER &= ~(3u << (2 * 5));
  GPIOB->MODER |= 2u << (2 * 5);

  // PA5, SCK
  GPIOA->MODER &= ~(3u << (2 * 5));
  GPIOA->MODER |= 2u << (2 * 5);

  // PA6, MISO
  GPIOA->MODER &= ~(3u << (2 * 6));
  GPIOA->MODER |= 2u << (2 * 6);

  // Step3: AFR (DS p.78-79, RM p.215-216)
  // AF5: 0101, AFR5 - 23:20, AFR6 - 27:24
  GPIOB->AFR[0] &= ~(0xFu << (4 * 5));
  GPIOB->AFR[0] |= 0x5u << (4 * 5);

  GPIOA->AFR[0] &= ~(0xFu << (4 * 5));
  GPIOA->AFR[0] |= 0x5u << (4 * 5);

  GPIOA->AFR[0] &= ~(0xFu << (4 * 6));
  GPIOA->AFR[0] |= 0x5u << (4 * 6);

  // Step4: SPI config (RM p.1132, 1084-1085)
  /*
   * CR1: BR, CPOL, CPHA, BIDIMODE=0, RXONLY=0, LSBFIRST=0, SSM=1, SSI=1, MSTR=1
   * CR2: DS=0111, FRXTH=1
   */

  // CR1
  // a) serial clock baud rate -> BR[2:0]
  uint32_t pclk2 = 16000000u;
  uint32_t target_rate = 4000000u;
  uint32_t br;
  for (br = 0; br <= 7; br++) {
    // BR = pclk2 / 2^(br+1)
    if (pclk2 >> (br + 1) <= target_rate) break;
  }
  if (br > 7)
    br = 7;
  
  // BR: CR1[3]
  SPI1->CR1 &= ~(0x7u << 3);
  SPI1->CR1 |= br << 3;

  // b) CPOL/CPHA
  SPI1->CR1 &= ~(1u); // CPHA = 0
  SPI1->CR1 &= ~(1 << 1); // CPOL = 0

  // c) Full-duplex/Half-duplex/Simplex mode (RXONLY/BIDIMODE/BIDIOE)
  /*
  * BIDIMODE: Bidirectional Data Mode Enable (0: full - 2 * simplex, 1: half - 1 * bi)
  * BIDIOE: Bidirectional mode Output Enable (if half-duplex - 0: receive, 1: transmit)
  * RXONLY (1: receive-only)
  */
  SPI1->CR1 &= ~(1 << 15); // BIDIMODE = 0
  SPI1->CR1 &= ~(1 << 10); // RXONLY = 0

  // d) LSBFIRST
  SPI1->CR1 &= ~(1 << 7); // LSBFIRST = 0, MSB -> LSB

  // e) if CRC - CRCL/CRCEN
  // f) SSM/SSI - NSS Management config
  // NSS - Not Slave Select, Chip selection

  SPI1->CR1 |= 1 << 9; // SSM - Software Slave Management
  SPI1->CR1 |= 1 << 8; // SSI - Internal Slave Select (NSS value)

  // g) MSTR (Master)
  SPI1->CR1 |= 1 << 2; 

  // CR2
  // a) DS[3:0] -> data length
  SPI1->CR2 &= ~(((1u << 4) - 1u) << 8);
  SPI1->CR2 |= ((1u << 3) - 1u) << 8; // 8bit frame

  // b) SSOE
  SPI1->CR2 &= ~(1 << 2);

  // c) if TI - FRF
  // d) if NSS pulse mode - FRF
  
  // e) FRXTH - FIFO Reception Threshold (RXFIFO <= size of DR)
  /*
  * FRXTH = 0: 32bit -(half)-> 16bit => RXNE = 1
  * FRXTH = 1: 32bit -(half * 2)-> 8bit => RXNE = 1
  */
  SPI1->CR2 |= 1u << 12; // FRXTH = 1

  // f) if DMA - LDMA_TX, LDMA_RX

  // if CRC - SPI_CRCPR (generator)

  // if DMA - TXDMAEN / RXDMAEN

  // Step5: enable SPI
  SPI1->CR1 |= 1u << 6;

  // Step6: Transmit & Receive
  // TODO: implement

  // Step7: Off
  // FTLVL: FIFO Transmission LeVeL (ft. TXE, RXNE)
  // FRLVL: FIFO Reception LeVeL
  while (((SPI1->SR >> 11) & 0x3u) != 0u) { } // TXFIFO empty
  while (SPI1->SR & (1u << 7)) { } // BSY == 0
  SPI1->CR1 &= ~(1u << 6);         // SPE = 0
  // read everything
  while (((SPI1->SR >> 9) & 0x3u) != 0u) {
    (void)*(volatile uint8_t *)&SPI1->DR;
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
#ifdef USE_FULL_ASSERT
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
