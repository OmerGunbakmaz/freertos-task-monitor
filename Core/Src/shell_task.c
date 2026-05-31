#include "shell_task.h"
#include "task_config.h"
#include "uart.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

#define CMD_BUF_LEN     64
#define PROMPT          "\r\n> "

static char s_buf[CMD_BUF_LEN];
static uint8_t s_len;

static void print(const char *str)
{
    UART2_SendString(str);
}

static void cmd_help(void)
{
    print("\r\n"
          "Commands:\r\n"
          "  tasks          show task list (name, state, priority, stack)\r\n"
          "  stats          show CPU runtime percentages\r\n"
          "  adc            show last ADC reading\r\n"
          "  led <mode>     set LED mode: idle | busy | error | off\r\n"
          "  uptime         ticks since boot\r\n"
          "  help           this message\r\n");
}

static void cmd_tasks(void)
{
    static char buf[512];
    vTaskList(buf);
    print("\r\nName            State  Pri  Stack  Num\r\n");
    print("----------------------------------------------\r\n");
    print(buf);
}

static void cmd_stats(void)
{
    static char buf[512];
    vTaskGetRunTimeStats(buf);
    print("\r\nName            Abs Time     % Time\r\n");
    print("--------------------------------------\r\n");
    print(buf);
}

static void cmd_adc(void)
{
    AdcData_t data;
    char buf[64];

    if (xQueuePeek(xAdcQueue, &data, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(buf, sizeof(buf), "\r\nTemp: %.1f C   Vref: %u mV\r\n",
                 (double)data.temperature_c, data.vref_mv);
    } else {
        snprintf(buf, sizeof(buf), "\r\nNo ADC data yet.\r\n");
    }
    print(buf);
}

static void cmd_uptime(void)
{
    char buf[48];
    uint32_t ticks = xTaskGetTickCount();
    snprintf(buf, sizeof(buf), "\r\nUptime: %lu ms\r\n", ticks);
    print(buf);
}

static void cmd_led(const char *arg)
{
    LedMode_t mode;

    if      (strcmp(arg, "idle")  == 0) mode = LED_IDLE;
    else if (strcmp(arg, "busy")  == 0) mode = LED_BUSY;
    else if (strcmp(arg, "error") == 0) mode = LED_ERROR;
    else if (strcmp(arg, "off")   == 0) mode = LED_OFF;
    else {
        print("\r\nUsage: led <idle|busy|error|off>\r\n");
        return;
    }

    xQueueOverwrite(xLedQueue, &mode);
    print("\r\nLED mode set.\r\n");
}

static void dispatch(void)
{
    if (s_len == 0) return;
    s_buf[s_len] = '\0';

    /* Split first word and optional argument */
    char *sp  = strchr(s_buf, ' ');
    char *arg = "";
    if (sp) {
        *sp = '\0';
        arg = sp + 1;
    }

    if      (strcmp(s_buf, "help")   == 0) cmd_help();
    else if (strcmp(s_buf, "tasks")  == 0) cmd_tasks();
    else if (strcmp(s_buf, "stats")  == 0) cmd_stats();
    else if (strcmp(s_buf, "adc")    == 0) cmd_adc();
    else if (strcmp(s_buf, "uptime") == 0) cmd_uptime();
    else if (strcmp(s_buf, "led")    == 0) cmd_led(arg);
    else {
        print("\r\nUnknown command. Type 'help'.\r\n");
    }
}

void vShellTask(void *pvParameters)
{
    (void)pvParameters;

    print("\r\n=== FreeRTOS Task Monitor ===\r\n");
    print("Type 'help' for commands.\r\n");
    print(PROMPT);

    uint8_t byte;
    s_len = 0;

    for (;;) {
        if (!UART2_ReadByte(&byte, 50)) continue;

        if (byte == '\r' || byte == '\n') {
            print("\r\n");
            dispatch();
            s_len = 0;
            print(PROMPT);
        } else if (byte == 0x7F || byte == '\b') {
            /* Backspace */
            if (s_len > 0) {
                s_len--;
                print("\b \b");
            }
        } else if (byte >= 0x20 && s_len < CMD_BUF_LEN - 1) {
            s_buf[s_len++] = (char)byte;
            UART2_SendBytes(&byte, 1);
        }
    }
}
