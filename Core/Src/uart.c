#include "uart.h"
#include "stm32f4xx_hal.h"
#include "task_config.h"
#include <string.h>

static UART_HandleTypeDef huart2;

void UART2_Init(uint32_t baud)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = baud;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void UART2_SendBytes(const uint8_t *buf, size_t len)
{
    if (xSemaphoreTake(xUartMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)len, HAL_MAX_DELAY);
        xSemaphoreGive(xUartMutex);
    }
}

void UART2_SendString(const char *str)
{
    UART2_SendBytes((const uint8_t *)str, strlen(str));
}

uint8_t UART2_ReadByte(uint8_t *out, uint32_t timeout_ms)
{
    return (HAL_UART_Receive(&huart2, out, 1, timeout_ms) == HAL_OK) ? 1 : 0;
}
