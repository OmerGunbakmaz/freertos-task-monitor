#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "task_config.h"
#include "uart.h"
#include "adc_task.h"
#include "led_task.h"
#include "shell_task.h"
#include "monitor_task.h"

QueueHandle_t    xAdcQueue;
QueueHandle_t    xLedQueue;
SemaphoreHandle_t xUartMutex;

static TimerHandle_t xHeartbeatTimer;

static void SystemClock_Config(void);
static void vHeartbeatCallback(TimerHandle_t xTimer);

/* TIM2 running at 10x tick rate for runtime stats */
void vConfigureTimerForRunTimeStats(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    TIM_HandleTypeDef htim2 = {0};
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = (SystemCoreClock / 10000) - 1;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 0xFFFFFFFF;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start(&htim2);
}

uint32_t ulGetRunTimeCounterValue(void)
{
    return TIM2->CNT;
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    UART2_Init(115200);

    xAdcQueue  = xQueueCreate(ADC_QUEUE_LEN,  sizeof(AdcData_t));
    xLedQueue  = xQueueCreate(LED_QUEUE_LEN,  sizeof(LedMode_t));
    xUartMutex = xSemaphoreCreateMutex();

    configASSERT(xAdcQueue);
    configASSERT(xLedQueue);
    configASSERT(xUartMutex);

    xTaskCreate(vMonitorTask, "Monitor", STACK_MONITOR, NULL, PRIORITY_MONITOR, NULL);
    xTaskCreate(vAdcTask,     "ADC",     STACK_ADC,     NULL, PRIORITY_ADC,     NULL);
    xTaskCreate(vLedTask,     "LED",     STACK_LED,     NULL, PRIORITY_LED,     NULL);
    xTaskCreate(vShellTask,   "Shell",   STACK_SHELL,   NULL, PRIORITY_SHELL,   NULL);

    /* Green LED (PD12) blinks at 1 Hz as long as the scheduler runs */
    xHeartbeatTimer = xTimerCreate("Heartbeat", pdMS_TO_TICKS(500), pdTRUE, NULL, vHeartbeatCallback);
    xTimerStart(xHeartbeatTimer, 0);

    vTaskStartScheduler();

    /* Never reached */
    for (;;) {}
}

static void vHeartbeatCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM       = 4;
    osc.PLL.PLLN       = 168;
    osc.PLL.PLLP       = RCC_PLLP_DIV2;
    osc.PLL.PLLQ       = 7;
    HAL_RCC_OscConfig(&osc);

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5);

    /* GPIO clocks for onboard LEDs (PD12-PD15) */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &gpio);
}
