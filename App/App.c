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

#define App_LPTIMER_INPUT_FREQUENCY_HZ 32768U
#define App_LPTIMER_PRESCALER 32U
#define App_LPTIMER_COUNTER_FREQUENCY_HZ (App_LPTIMER_INPUT_FREQUENCY_HZ / App_LPTIMER_PRESCALER)
#define App_LPTIMER_TICKS_PER_MS (App_LPTIMER_COUNTER_FREQUENCY_HZ / 1000U)
#define App_LPTIMER_TICK_INTERVAL_MS 1U
#define App_LPTIMER_MAX_PERIOD 0xFFFFU

/* Flag set from the tick interrupt to allow the sleep processing to know if
 * sleep mode was exited because of an tick interrupt or a different interrupt. 
 */

static volatile uint8_t ucTickFlag = pdFALSE;

static void prvLEDBLINKTask(void *pvParameters);
static void prvLOADTask(void *pvParameters);
void xPortSysTickHandler( void );

/**
 * @brief  Initialise the timer used by the FreeRTOS kernel for the tick.
 *
 * The LPTIM1 peripheral is used instead of the default SysTick to allow
 * the MCU to enter low‑power STOP states.  This routine is called from
 * the FreeRTOS port layer when the scheduler starts.
 *
 * The debugger freeze macro prevents the LPTIM from running when the core
 * is halted, so the tick count does not advance during a debug pause.
 * The HAL API programs a timeout corresponding to one tick period
 * (derived from \c configTICK_RATE_HZ) and enables its interrupt.
 */
void vPortSetupTimerInterrupt(void) {
    __HAL_DBGMCU_FREEZE_LPTIM1();
    HAL_LPTIM_TimeOut_Start_IT(&hlptim1, App_LPTIMER_MAX_PERIOD, App_LPTIMER_TICKS_PER_MS * App_LPTIMER_TICK_INTERVAL_MS);
}

/* */
void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        /* Call tick handler */
        xPortSysTickHandler();
        HAL_LPTIM_TimeOut_Start_IT(&hlptim1, App_LPTIMER_MAX_PERIOD, App_LPTIMER_TICKS_PER_MS * App_LPTIMER_TICK_INTERVAL_MS);
        ucTickFlag = pdFALSE;
    }
}

/**
 * @brief   Suppress the RTOS tick and enter a low‑power STOP1 state.
 *
 * This function is called from the FreeRTOS kernel when the scheduler
 * determines that the system can remain idle for at least
 * \c xExpectedIdleTime tick periods.  The normal SysTick interrupt is
 * stopped and the 16‑bit LPTIM1 peripheral is programmed to generate a
 * wake‑up interrupt after the requested number of ticks.  The CPU is then
 * put into STOP1 mode using HAL_PWREx_EnterSTOP1Mode().
 *
 * When the device wakes (either because the LPTIM interrupt occurred or
 * some other interrupt fired), the clock configuration is restored, the
 * number of complete ticks that elapsed while sleeping is calculated and
 * added to the kernel tick count with vTaskStepTick(), and the SysTick
 * timer is restarted.  If a task becomes ready while sleep preparations
 * are under way, sleep is aborted and the function returns without
 * entering low‑power mode.
 *
 * @param[in] xExpectedIdleTime Number of tick periods that the kernel
 *            expects the system to be idle; limited to 0xFFFF to fit the
 *            LPTIM timeout register.
 *
 * @note    The code assumes a 1024 Hz LPTIM clock (prescaler = 1024).
 *          Adjust the conversion calculations if the clock setup changes.
 */
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
        // HAL_GPIO_TogglePin(BSP_LED_RED_GPIO_Port, BSP_LED_RED_Pin);
        //HAL_Delay(2000);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime){
    uint32_t ulCompleteTickPeriods = 0U;
    uint32_t lptimCounts;
    eSleepModeStatus sleepStatus;

    /* Limit maximum suppressible time */
    if (xExpectedIdleTime > App_LPTIMER_MAX_PERIOD)
        xExpectedIdleTime = App_LPTIMER_MAX_PERIOD;

    __disable_irq(); // Ensure no interrupts can interfere with the sleep preparations

    sleepStatus = eTaskConfirmSleepModeStatus();

    if (sleepStatus == eAbortSleep){
        HAL_LPTIM_TimeOut_Start_IT(&hlptim1, App_LPTIMER_MAX_PERIOD, App_LPTIMER_TICKS_PER_MS * App_LPTIMER_TICK_INTERVAL_MS);
        __enable_irq(); // Sleep aborted, re-enable interrupts and return
        return;
    }else if (sleepStatus == eNoTasksWaitingTimeout) {
        HAL_SuspendTick();
        /* Enter STOP1 */
        HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);
        SystemClock_Config(); // Reconfigure system clock after waking up from STOP1 mode
        HAL_ResumeTick();
    }else{
        HAL_SuspendTick();

        HAL_LPTIM_TimeOut_Stop_IT(&hlptim1); // Stop the timer to prevent it from firing during sleep preparations

        /*  The tick flag is set to false before sleeping. If it is true when sleep
            mode is exited then sleep mode was probably exited because the tick was
            suppressed for the entire xExpectedIdleTime period. */
        ucTickFlag = pdFALSE;

        __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARRM);

        HAL_LPTIM_TimeOut_Start_IT(&hlptim1, App_LPTIMER_MAX_PERIOD, App_LPTIMER_TICKS_PER_MS * xExpectedIdleTime);

        HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI); // Enter STOP1 mode, will wake on LPTIM interrupt or any other enabled interrupt
        
        SystemClock_Config(); // Reconfigure system clock after waking up from STOP1 mode
        
        //HAL_LPTIM_TimeOut_Stop_IT(&hlptim1); // Stop the timer to prevent it from firing during sleep preparations
        
        __enable_irq();

        //HAL_LPTIM_TimeOut_Stop_IT(&hlptim1); // 

        if (ucTickFlag != pdFALSE) {
            /* The tick interrupt is already pending, but the tick count has not yet been incremented. */
            ulCompleteTickPeriods = xExpectedIdleTime;
        } else {
            /* The tick interrupt did not occur, but sleep was still exited.  This could be because
               an interrupt other than the tick interrupt caused sleep to exit.  In this case
               the timer may have been stopped by the interrupt handler, or it may have continued
               running.  Either way, we cannot be sure how many tick periods passed while the
               processor was asleep.  The best we can do is assume the maximum possible tick periods
               passed, which is the value of xExpectedIdleTime. */

            /* Calculate the number of complete tick periods that elapsed while sleeping. */
            lptimCounts = xExpectedIdleTime;//hlptim1.Instance->CNT; //HAL_LPTIM_ReadCounter(&hlptim1);

            ulCompleteTickPeriods = (lptimCounts / App_LPTIMER_TICKS_PER_MS);

            if(ulCompleteTickPeriods == 0U){
                /* The timer did not count enough to generate a tick interrupt, but sleep was still exited.
                  This could be because an interrupt other than the tick interrupt caused sleep to exit, or 
                  because the timer was stopped by an interrupt handler before it could count enough to generate a tick interrupt.
                  In this case, we cannot be sure how many tick periods passed while the processor was asleep. 
                  The best we can do is assume at least one tick period passed, but not more than xExpectedIdleTime. */
                HAL_LPTIM_TimeOut_Start_IT(&hlptim1, App_LPTIMER_MAX_PERIOD, App_LPTIMER_TICKS_PER_MS * App_LPTIMER_TICK_INTERVAL_MS);
            }else{
                if (ulCompleteTickPeriods > xExpectedIdleTime) {
                    ulCompleteTickPeriods = xExpectedIdleTime;
                }
                HAL_LPTIM_TimeOut_Start_IT(&hlptim1, App_LPTIMER_MAX_PERIOD, App_LPTIMER_TICKS_PER_MS * (xExpectedIdleTime - ulCompleteTickPeriods));
            }
        }
    }

    /* MCU resumes here after interrupt */
    HAL_ResumeTick();

    /* Wind the tick forward by the number of tick periods that the CPU remained in a low power state. */
    vTaskStepTick(ulCompleteTickPeriods);
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