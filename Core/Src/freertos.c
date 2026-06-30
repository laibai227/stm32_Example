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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include <stdio.h>
#include "My_key.h"
#include "string.h"
#include "W25Q16.h"
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
QueueHandle_t xKeyQueue = NULL;

/* 队列静态内存 */
static StaticQueue_t xKeyQueueTCB;
static uint8_t xKeyQueueStorage[5 * sizeof(KeyType)];

/* KeyTask 静态内存 */
static StaticTask_t xKeyTaskTCB;
static StackType_t xKeyTaskStack[128];
TaskHandle_t xKeyTaskHandle = NULL;

/* ControlTask 静态内存 */
static StaticTask_t xControlTaskTCB;
static StackType_t xControlTaskStack[192];
TaskHandle_t xControlTaskHandle = NULL;
/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void xKeyTask(void *argument);
void xControlTask(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  //初始化OLED显示屏
  OLED_Init();
  
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  xKeyQueue = xQueueCreateStatic(
  5,
  sizeof(KeyType),
  xKeyQueueStorage,
  &xKeyQueueTCB
);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  xKeyTaskHandle = xTaskCreateStatic(
    xKeyTask,
    "KeyTask",
    128,
    NULL,
    tskIDLE_PRIORITY + 2,
    xKeyTaskStack,
    &xKeyTaskTCB
  );

  xControlTaskHandle = xTaskCreateStatic(
    xControlTask,
    "CtrlTask",
    192,
    NULL,
    tskIDLE_PRIORITY + 1,
    xControlTaskStack,
    &xControlTaskTCB
  );
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


void xKeyTask(void *argument)
{
  KeyType key;

  for(;;)
  {
    key = Key_Scan(GPIOB, KEY2_PIN);

    if(key != KEY_NONE)
    {
        xQueueSend(xKeyQueue, &key, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void xControlTask(void *argument)
{
  KeyType key;

  uint8_t led_mode = 0;
  uint32_t last_tick = 0;
  uint32_t blink_period = 0;

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

  OLED_Show_String(0, 0, "READY   ", 8);
  OLED_Show_String(0, 16, "MODE:OFF", 8);
  OLED_Refresh_Gram();

  for(;;)
  {
    if(xQueueReceive(xKeyQueue, &key, pdMS_TO_TICKS(10)) == pdTRUE)
    {
      if(key == KEY_PRESS)
      {
        if(led_mode == 0)
        {
          led_mode = 1;
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // LED ON
          OLED_Show_String(0, 16, "MODE:ON ", 8);
          OLED_Refresh_Gram();
        }
        else
        {
          led_mode = 0;
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);    // LED灭

          OLED_Show_String(0, 16, "MODE:OFF", 8);
          OLED_Refresh_Gram();
        }
      }
      else if(key == KEY_SHORT)
      {
        led_mode = 2;
        blink_period = 2000;
        last_tick = HAL_GetTick();

        OLED_Show_String(0, 16, "MODE:2S ", 8);
        OLED_Refresh_Gram();
      }
      else if(key == KEY_DOUBLE)
      {
        led_mode = 3;
        blink_period = 1000;
        last_tick = HAL_GetTick();

        OLED_Show_String(0, 16, "MODE:1S ", 8);
        OLED_Refresh_Gram();
      }
      else if(key == KEY_LONG)
      {
        led_mode = 4;
        blink_period = 300;
        last_tick = HAL_GetTick();

        OLED_Show_String(0, 16, "MODE:FST", 8);
        OLED_Refresh_Gram();
      }
    }

    if(led_mode == 0)
    {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);    // LED灭
    }
    else if(led_mode == 1)
    {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);  // LED亮
    }
    else if(led_mode == 2 || led_mode == 3 || led_mode == 4)
    {
      if((HAL_GetTick() - last_tick) >= blink_period)
      {
          last_tick = HAL_GetTick();
          HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
      }
    }
  }
}



// 队列任务排队执行示例
// void xControlTask(void *argument)
// {
//     KeyType key;
//     UBaseType_t queue_count;

//     uint8_t led_mode = 0;
//     uint32_t last_tick = 0;
//     uint32_t blink_period = 0;

//     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

//     OLED_Show_String(0, 0, "READY   ", 8);
//     OLED_Show_String(0, 16, "MODE:OFF", 8);
//     OLED_Refresh_Gram();

//     for(;;)
//     {
//         /*
//          * 等待按键事件
//          */
//         if(xQueueReceive(xKeyQueue, &key, portMAX_DELAY) == pdTRUE)
//         {
//             /*
//              * 显示当前队列里还剩几个事件
//              */
//             queue_count = uxQueueMessagesWaiting(xKeyQueue);

//             if(key == KEY_PRESS)
//             {
//                 if(led_mode == 0)
//                 {
//                     led_mode = 1;
//                     OLED_Show_String(0, 16, "MODE:ON ", 8);
//                 }
//                 else
//                 {
//                     led_mode = 0;
//                     OLED_Show_String(0, 16, "MODE:OFF", 8);
//                 }
//             }
//             else if(key == KEY_SHORT)
//             {
//                 led_mode = 2;
//                 blink_period = 2000;
//                 last_tick = HAL_GetTick();

//                 OLED_Show_String(0, 16, "MODE:2S ", 8);
//             }
//             else if(key == KEY_DOUBLE)
//             {
//                 led_mode = 3;
//                 blink_period = 1000;
//                 last_tick = HAL_GetTick();

//                 OLED_Show_String(0, 16, "MODE:1S ", 8);
//             }
//             else if(key == KEY_LONG)
//             {
//                 led_mode = 4;
//                 blink_period = 300;
//                 last_tick = HAL_GetTick();

//                 OLED_Show_String(0, 16, "MODE:FST", 8);
//             }

//             OLED_Show_String(0, 32, "DOING   ", 8);
//             OLED_Printf(0, 48, 8, "QUEUE:%d", queue_count);
//             OLED_Refresh_Gram();

//             /*
//              * 故意模拟一个耗时任务
//              * 这 3 秒内你继续按键，KeyTask 还会继续扫描并发送队列
//              */
//             vTaskDelay(pdMS_TO_TICKS(3000));

//             OLED_Show_String(0, 32, "DONE    ", 8);
//             OLED_Refresh_Gram();
//         }

//         /*
//          * 根据 led_mode 控制 LED
//          */
//         if(led_mode == 0)
//         {
//             HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
//         }
//         else if(led_mode == 1)
//         {
//             HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
//         }
//         else if(led_mode == 2 || led_mode == 3 || led_mode == 4)
//         {
//             if((HAL_GetTick() - last_tick) >= blink_period)
//             {
//                 last_tick = HAL_GetTick();
//                 HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
//             }
//         }
//     }
// }
