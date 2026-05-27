#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* Task priorities */
#define PRIORITY_MONITOR    5
#define PRIORITY_ADC        3
#define PRIORITY_LED        3
#define PRIORITY_SHELL      2

/* Stack sizes (words) */
#define STACK_MONITOR       256
#define STACK_ADC           256
#define STACK_LED           128
#define STACK_SHELL         512

/* Queue lengths */
#define ADC_QUEUE_LEN       1
#define LED_QUEUE_LEN       1
#define SHELL_TX_QUEUE_LEN  16

/* Shared handles — defined in main.c */
extern QueueHandle_t  xAdcQueue;
extern QueueHandle_t  xLedQueue;
extern SemaphoreHandle_t xUartMutex;

/* LED command codes */
typedef enum {
    LED_IDLE   = 0,
    LED_BUSY   = 1,
    LED_ERROR  = 2,
    LED_OFF    = 3,
} LedMode_t;

/* ADC data packet */
typedef struct {
    float    temperature_c;
    uint16_t vref_mv;
} AdcData_t;
