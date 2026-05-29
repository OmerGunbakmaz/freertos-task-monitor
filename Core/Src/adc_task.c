#include "adc_task.h"
#include "task_config.h"
#include "stm32f4xx_hal.h"

/*
 * STM32F4 internal sensors are on ADC1 channels 16 (temp) and 17 (Vref).
 * We do two sequential conversions and map raw values using the factory
 * calibration data stored in the system memory at 0x1FFF7A2C / 0x1FFF7A2A.
 */

#define VDDA_MV             3300U
#define ADC_MAX             4095U
#define TS_CAL1_ADDR        ((uint16_t *)0x1FFF7A2CUL)  /* 30°C  */
#define TS_CAL2_ADDR        ((uint16_t *)0x1FFF7A2EUL)  /* 110°C */
#define VREFINT_CAL_ADDR    ((uint16_t *)0x1FFF7A2AUL)

static ADC_HandleTypeDef hadc1;

static void adc_init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = ENABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 2;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};

    ch.Channel      = ADC_CHANNEL_TEMPSENSOR;
    ch.Rank         = 1;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    ch.Channel      = ADC_CHANNEL_VREFINT;
    ch.Rank         = 2;
    ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    ADC->CCR |= ADC_CCR_TSVREFE;
}

static uint16_t adc_read_channel(uint32_t channel, uint32_t rank)
{
    (void)rank;
    (void)channel;
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    return (uint16_t)HAL_ADC_GetValue(&hadc1);
}

void vAdcTask(void *pvParameters)
{
    (void)pvParameters;

    adc_init();

    uint16_t ts_cal1   = *TS_CAL1_ADDR;
    uint16_t ts_cal2   = *TS_CAL2_ADDR;
    uint16_t vref_cal  = *VREFINT_CAL_ADDR;

    TickType_t xLastWake = xTaskGetTickCount();

    for (;;) {
        uint16_t raw_temp = adc_read_channel(ADC_CHANNEL_TEMPSENSOR, 1);
        uint16_t raw_vref = adc_read_channel(ADC_CHANNEL_VREFINT,    2);

        float temperature = (float)(raw_temp - ts_cal1) *
                            (110.0f - 30.0f) / (float)(ts_cal2 - ts_cal1) + 30.0f;

        uint16_t vref_mv = (uint16_t)((VDDA_MV * vref_cal) / raw_vref);

        AdcData_t data = {
            .temperature_c = temperature,
            .vref_mv       = vref_mv,
        };

        xQueueOverwrite(xAdcQueue, &data);

        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(1000));
    }
}
