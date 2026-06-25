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
#include "cmsis_os2.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "HardWare_Init_Task.h"
#include "HeartRate_Task.h"
#include "LVGL_Task.h"
#include "Power_Task.h"
#include "Print_Task.h"
#include "Sensor_Task.h"
#include "Watchdog_Task.h"
#include "freertos_debug.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WATCHDOG_TASK_ENABLE 0U
#define HEART_RATE_UART_TX_TASK_ENABLE 0U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* Definitions for watchdogTask */
osThreadId_t watchdogTaskHandle;
const osThreadAttr_t watchdogTask_attributes = {
  .name = "watchdogTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};

/* Definitions for sensorTask */
osThreadId_t sensorTaskHandle;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};

/* Definitions for powerTask */
osThreadId_t powerTaskHandle;
const osThreadAttr_t powerTask_attributes = {
  .name = "powerTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};

/* Definitions for heartRateTask */
osThreadId_t heartRateTaskHandle;
const osThreadAttr_t heartRateTask_attributes = {
  .name = "heartRateTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};

/* Definitions for heartRateTxTask */
osThreadId_t heartRateTxTaskHandle;
const osThreadAttr_t heartRateTxTask_attributes = {
  .name = "heartRateTxTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Definitions for printTask */
osThreadId_t printTaskHandle;
const osThreadAttr_t printTask_attributes = {
  .name = "printTask",
  .stack_size = (256 * 4) + 128,
  .priority = (osPriority_t) osPriorityLow,
};

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Definitions for hardWareInitTask */
osThreadId_t hardWareInitTaskHandle;
const osThreadAttr_t hardWareInitTask_attributes = {
  .name = "hwInitTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};

/* Definitions for lvglTask */
osThreadId_t lvglTaskHandle;
const osThreadAttr_t lvglTask_attributes = {
  .name = "lvglTask",
  .stack_size = 1024 * 6,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for keyTask */
osThreadId_t keyTaskHandle;
const osThreadAttr_t keyTask_attributes = {
  .name = "keyTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Key_Task(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  Power_Task_InitObjects();
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* creation of hardWareInitTask */
  hardWareInitTaskHandle = osThreadNew(HardWare_Init_Task, NULL, &hardWareInitTask_attributes);

  /* creation of lvglTask */
  lvglTaskHandle = osThreadNew(LVGL_Task, NULL, &lvglTask_attributes);

  /* creation of keyTask */
  keyTaskHandle = osThreadNew(Key_Task, NULL, &keyTask_attributes);

  /* creation of sensorTask */
  sensorTaskHandle = osThreadNew(Sensor_Task, NULL, &sensorTask_attributes);

  /* creation of powerTask */
  powerTaskHandle = osThreadNew(Power_Task, NULL, &powerTask_attributes);

  /* creation of printTask */
  printTaskHandle = osThreadNew(Print_Task, NULL, &printTask_attributes);

  /* creation of heartRateTask */
  heartRateTaskHandle = osThreadNew(HeartRate_Task, NULL, &heartRateTask_attributes);

  /* creation of heartRateTxTask */
#if (HEART_RATE_UART_TX_TASK_ENABLE != 0U)
  heartRateTxTaskHandle = osThreadNew(HeartRate_UartTx_Task, NULL, &heartRateTxTask_attributes);
#else
  heartRateTxTaskHandle = NULL;
#endif

  /* creation of watchdogTask */
#if (WATCHDOG_TASK_ENABLE != 0U)
  watchdogTaskHandle = osThreadNew(Watchdog_Task, NULL, &watchdogTask_attributes);
#else
  // 调试期间关闭外部看门狗任务，避免断点暂停时复位；恢复时将 WATCHDOG_TASK_ENABLE 改为 1U。
  watchdogTaskHandle = NULL;
#endif

  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)defaultTaskHandle, defaultTask_attributes.name);
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)hardWareInitTaskHandle, hardWareInitTask_attributes.name);
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)lvglTaskHandle, lvglTask_attributes.name);
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)keyTaskHandle, keyTask_attributes.name);
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)sensorTaskHandle, sensorTask_attributes.name);
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)powerTaskHandle, powerTask_attributes.name);
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)printTaskHandle, printTask_attributes.name);
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)heartRateTaskHandle, heartRateTask_attributes.name);
#if (HEART_RATE_UART_TX_TASK_ENABLE != 0U)
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)heartRateTxTaskHandle, heartRateTxTask_attributes.name);
#endif
#if (WATCHDOG_TASK_ENABLE != 0U)
  (void)FreeRTOS_Debug_RegisterTask((TaskHandle_t)watchdogTaskHandle, watchdogTask_attributes.name);
#endif
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  (void)argument;

  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */


