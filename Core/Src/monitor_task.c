#include "monitor_task.h"
#include "task_config.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

#define MAX_TASKS   16

/*
 * Runs at the highest application priority so it can always get CPU time
 * to collect stats, but spends most of its time blocked in vTaskDelayUntil.
 *
 * Every 10 seconds it prints a compact status table over UART so the user
 * can see what the system is doing without issuing the 'tasks' command.
 */
void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    static TaskStatus_t tasks[MAX_TASKS];
    static char line[80];

    TickType_t xLastWake = xTaskGetTickCount();
    uint32_t   ulTotalRunTime;

    for (;;) {
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(10000));

        UBaseType_t count = uxTaskGetSystemState(tasks, MAX_TASKS, &ulTotalRunTime);

        UART2_SendString("\r\n--- monitor ---\r\n");
        UART2_SendString("Name            Pri  Stack(w)  CPU%\r\n");

        for (UBaseType_t i = 0; i < count; i++) {
            uint32_t cpu_pct = (ulTotalRunTime > 0)
                ? (tasks[i].ulRunTimeCounter * 100UL) / ulTotalRunTime
                : 0;

            snprintf(line, sizeof(line), "%-15s %3lu  %8lu  %3lu%%\r\n",
                     tasks[i].pcTaskName,
                     (unsigned long)tasks[i].uxCurrentPriority,
                     (unsigned long)tasks[i].usStackHighWaterMark,
                     (unsigned long)cpu_pct);

            UART2_SendString(line);
        }

        snprintf(line, sizeof(line), "Heap free: %u bytes\r\n",
                 (unsigned)xPortGetFreeHeapSize());
        UART2_SendString(line);
    }
}
