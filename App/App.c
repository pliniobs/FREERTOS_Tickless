#include "App.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_lptim.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* STM32 Specific Includes */
#include "stm32l4xx_hal.h"
#include "main.h"
#include "gpio.h"
#include "usart.h"
#include "lptim.h"

static void prvLEDBLINKTask(void *pvParameters);
static void prvLOADTask(void *pvParameters);

void App_Init(void){

    /* Create the queue receive task as described in the comments at the top
    of this    file. */
    if(xTaskCreate(/* The function that implements the task. */
                prvLEDBLINKTask,
                /* Text name for the task, just to help debugging. */
                "LED BLINK",
                /* The size (in words) of the stack that should be created
                for the task. */
                configMINIMAL_STACK_SIZE + 256,
                /* A parameter that can be passed into the task.  Not used
                in this simple demo. */
                NULL,
                /* The priority to assign to the task.  tskIDLE_PRIORITY
                (which is 0) is the lowest priority.  configMAX_PRIORITIES - 1
                is the highest priority. */
                tskIDLE_PRIORITY + 2,
                /* Used to obtain a handle to the created task.  Not used in
                this simple demo, so set to NULL. */
                NULL) != pdPASS){
                    while (1);
                }


    if(xTaskCreate(/* The function that implements the task. */
                prvLOADTask,
                /* Text name for the task, just to help debugging. */
                "LOAD TASK",
                /* The size (in words) of the stack that should be created
                for the task. */
                configMINIMAL_STACK_SIZE + 256,
                /* A parameter that can be passed into the task.  Not used
                in this simple demo. */
                NULL,
                /* The priority to assign to the task.  tskIDLE_PRIORITY
                (which is 0) is the lowest priority.  configMAX_PRIORITIES - 1
                is the highest priority. */
                tskIDLE_PRIORITY + 2,
                /* Used to obtain a handle to the created task.  Not used in
                this simple demo, so set to NULL. */
                NULL) != pdPASS){
                    while (1);
                }
}

static void prvLEDBLINKTask(void *pvParameters){
    
    for(;;){
        /* Toggle LED */
        HAL_GPIO_TogglePin(BSP_LED_BLUE_GPIO_Port, BSP_LED_BLUE_Pin);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
static void prvLOADTask(void *pvParameters)
{
    for(;;){
        /* Simulate load */
        //HAL_GPIO_TogglePin(BSP_LED_RED_GPIO_Port, BSP_LED_RED_Pin);
        HAL_Delay(2000);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void PreSleepProcessing(uint32_t ulExpectedIdleTime)
{
/* place for user code */
    HAL_GPIO_WritePin(BSP_LED_RED_GPIO_Port, BSP_LED_RED_Pin, GPIO_PIN_SET);
    HAL_SuspendTick();
    HAL_LPTIM_TimeOut_Start(&hlptim1, 0xFFFF, ulExpectedIdleTime);
    HAL_PWREx_EnterSTOP1Mode(PWR_SLEEPENTRY_WFI);
}

void PostSleepProcessing(uint32_t ulExpectedIdleTime)
{
/* place for user code */
    HAL_GPIO_WritePin(BSP_LED_RED_GPIO_Port, BSP_LED_RED_Pin, GPIO_PIN_RESET);
    HAL_LPTIM_TimeOut_Stop_IT(&hlptim1);
    HAL_ResumeTick();
}