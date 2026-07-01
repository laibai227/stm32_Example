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

// 任务1 参数
#define TASK1_STACK_SIZE 128
#define TASK1_PRIORITY 5
StackType_t Task1Stack[TASK1_STACK_SIZE];
StaticTask_t Task1TCB;
TaskHandle_t Task1Handle;

// 任务2 参数
#define TASK2_STACK_SIZE 128
#define TASK2_PRIORITY 5
StackType_t Task2Stack[TASK2_STACK_SIZE];
StaticTask_t Task2TCB;
TaskHandle_t Task2Handle;

// 任务3 参数
#define TASK3_STACK_SIZE 128
#define TASK3_PRIORITY 5
StackType_t Task3Stack[TASK3_STACK_SIZE];
StaticTask_t Task3TCB;
TaskHandle_t Task3Handle;

// 队列
#define QUEUE1_LENGTH 5
#define QUEUE1_ITEM_SIZE sizeof(int)
uint8_t Queue1StorageBuffer[QUEUE1_LENGTH * QUEUE1_ITEM_SIZE];
QueueHandle_t Queue1Handle;
StaticQueue_t Queue1Buffer;

#define BIG_QUEUE_LENGTH 5
#define BIG_QUEUE_ITEM_SIZE sizeof(uint32_t *)
uint8_t BigQueueStorageBuffer[BIG_QUEUE_LENGTH * BIG_QUEUE_ITEM_SIZE];
QueueHandle_t BigQueueHandle;
StaticQueue_t BigQueueBuffer;

// 1. 定义大数据结构体（比如 1KB）
typedef struct
{
  uint32_t id;
  uint8_t payload[1024]; // 1KB 数据
} LargeData_t;

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void task1(void *argument);
void task2(void *argument);
void task3(void *argument);

/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
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
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  // 初始化OLED显示屏
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
  Queue1Handle = xQueueCreateStatic(QUEUE1_LENGTH, QUEUE1_ITEM_SIZE, Queue1StorageBuffer, &Queue1Buffer);
  BigQueueHandle = xQueueCreateStatic(BIG_QUEUE_LENGTH, BIG_QUEUE_ITEM_SIZE, BigQueueStorageBuffer, &BigQueueBuffer);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  xTaskCreateStatic(task1,
                    "task1",
                    TASK1_STACK_SIZE,
                    NULL,
                    TASK1_PRIORITY,
                    Task1Stack,
                    &Task1TCB);
  xTaskCreateStatic(task2,
                    "task2",
                    TASK2_STACK_SIZE,
                    NULL,
                    TASK2_PRIORITY,
                    Task2Stack,
                    &Task2TCB);
  xTaskCreateStatic(task3,
                    "task3",
                    TASK3_STACK_SIZE,
                    NULL,
                    TASK3_PRIORITY,
                    Task3Stack,
                    &Task3TCB);
  /* USER CODE END RTOS_THREADS */
}

void task1(void *argument)
{
  KeyType key1, key2;
  uint32_t count = 0;

  while (1)
  {
    key1 = Key_Scan(GPIOB, KEY1_PIN);
    key2 = Key_Scan(GPIOB, KEY2_PIN);

    if (key1 != KEY_NONE)
    {
      xQueueSend(Queue1Handle, &key1, 10);
    }

    if (key2 == KEY_PRESS)
    {
      // 每次发送时重新申请内存并填充
      LargeData_t *pData = (LargeData_t *)pvPortMalloc(sizeof(LargeData_t));
      if (pData != NULL)
      {
        pData->id = count++;
        for (int i = 0; i < sizeof(pData->payload); i++)
        {
          pData->payload[i] = (uint8_t)(count & 0xFF);
        }
        // 发送指针的地址
        if (xQueueSend(BigQueueHandle, &pData, 10) != pdPASS)
        {
          // 处理发送失败的情况
          vPortFree(pData);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void task2(void *argument)
{
  KeyType key1;
  while (1)
  {
    xQueueReceive(Queue1Handle, &key1, 10);
    // 显示按键类型
    OLED_Show_String(0, 0, "KEY:", 8);
    OLED_ShowNum(40, 0, key1, 1, 8);
    OLED_Refresh_Gram();
  }
}

void task3(void *argument)
{
  uint16_t i = 0;
  LargeData_t *ReceivedData;
  while (1)
  {
    if (xQueueReceive(BigQueueHandle, &ReceivedData, 10) == pdPASS)
    {
      // 处理接收到的数据
      // ...
      OLED_Printf(0, 20, 8, "ID:%d", ReceivedData->id);
      OLED_Printf(0, 30, 8, "Val:%d", ReceivedData->payload[i]); // 显示 payload 的第一个字节
      i++;
      OLED_Refresh_Gram();
      // 处理完毕后释放内存
      vPortFree(ReceivedData);
      ReceivedData = NULL;
    }
  }
}
