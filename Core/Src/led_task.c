#include "led_task.h"
#include "task_config.h"
#include "stm32f4xx_hal.h"

/*
 * Discovery board LEDs:
 *   PD12 — Green   (heartbeat, driven by software timer in main.c)
 *   PD13 — Orange  (idle pattern)
 *   PD14 — Red     (error pattern)
 *   PD15 — Blue    (busy pattern)
 */

static void all_off(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
}

void vLedTask(void *pvParameters)
{
    (void)pvParameters;

    LedMode_t mode    = LED_IDLE;
    LedMode_t newMode = LED_IDLE;

    for (;;) {
        /* Non-blocking receive — update mode if a new command arrived */
        if (xQueueReceive(xLedQueue, &newMode, 0) == pdTRUE) {
            mode = newMode;
            all_off();
        }

        switch (mode) {
            case LED_IDLE:
                /* Orange slow blink (1 Hz) */
                HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case LED_BUSY:
                /* Blue fast blink (4 Hz) */
                HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
                vTaskDelay(pdMS_TO_TICKS(125));
                break;

            case LED_ERROR:
                /* Red double-blink then pause */
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
                vTaskDelay(pdMS_TO_TICKS(100));
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
                vTaskDelay(pdMS_TO_TICKS(100));
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
                vTaskDelay(pdMS_TO_TICKS(100));
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
                vTaskDelay(pdMS_TO_TICKS(700));
                break;

            case LED_OFF:
                all_off();
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}
