#pragma once

#include <stdint.h>
#include <stddef.h>

void UART2_Init(uint32_t baud);
void UART2_SendString(const char *str);
void UART2_SendBytes(const uint8_t *buf, size_t len);
uint8_t UART2_ReadByte(uint8_t *out, uint32_t timeout_ms);
