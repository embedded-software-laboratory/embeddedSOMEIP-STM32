/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

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
/* USER CODE BEGIN Variables */
// Heap sits in D2 SRAM to keep it out of the near-full RAM_D1.
// Needs configAPPLICATION_ALLOCATED_HEAP and the D2 clocks enabled in main.c.
__attribute__((section(".ram_d2"))) uint8_t ucHeap[configTOTAL_HEAP_SIZE];
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  volatile TaskHandle_t task = xTask;
  volatile char *name = pcTaskName;
  (void)task;
  (void)name;
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
    /* Inspect `name` here to see which task overflowed. */
  }
}

void vApplicationMallocFailedHook(void)
{
  volatile size_t freeHeap = xPortGetFreeHeapSize();
  (void)freeHeap;
  taskDISABLE_INTERRUPTS();
  for (;;)
  {
    // heap exhausted, raise configTOTAL_HEAP_SIZE
  }
}

/* USER CODE END Application */

