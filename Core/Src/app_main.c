/**
 * @file app_main.c
 * @author
 * @brief Application code built on the Nucleo-F411RE board and STM32Cube
 * autogenerated code.
 * @date 2024-06-27
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "stm32f4xx_hal_uart.h"

extern void SystemClock_Config(void); // grabs auto-generated function from main.c
static UART_HandleTypeDef huart1;
static UART_HandleTypeDef huart2;

static void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void) {
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);
}

void TransceiveCharacterByCharacter(UART_HandleTypeDef *huart, uint8_t *message, uint8_t *response,
				    uint16_t response_size) {
  uint16_t index = 0;
  while (*message) {
    (void)HAL_UART_Transmit(huart, message, 1, 10);

    if (index < response_size - 1) {
      assert(HAL_ERROR != HAL_UART_Receive(huart, &response[index], 1, 1000));
      index++;
    }

    message++;
  }
  response[index] = '\0';
}

void UART_Printf(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
}

uint8_t *AT_Command(UART_HandleTypeDef *huart, uint8_t *command, uint8_t *response, uint16_t response_size) {
  uint8_t fmt_cmd[128] = {0};
  snprintf(fmt_cmd, sizeof(fmt_cmd), "%s\r\n", command); // todo check error

  uint8_t echo[128] = {0};
  TransceiveCharacterByCharacter(huart, fmt_cmd, echo, sizeof(echo));
  if (strncmp((char *)command, (char *)echo, strlen((char *)command)) != 0) {
    return NULL;
  }

  uint8_t interstitial_control[3] = {0};
  if (HAL_OK != HAL_UART_Receive(huart, interstitial_control, 3, 1000)) {
    return NULL;
  } else if (strncmp("\n\r\n", interstitial_control, sizeof(interstitial_control))) {
    return NULL;
  }

  uint16_t index = 0;
  uint8_t prev_ch;
  while (index < response_size - 1) {
    uint8_t ch;
    if (HAL_UART_Receive(huart, &ch, 1, 1000) != HAL_OK) {
      return NULL;
    }

    if (ch == '\r') {
      prev_ch = ch;
      continue;
    } else if (ch == '\n' && prev_ch == '\r') {
      response[index] = '\0';
      return response;
    }

    response[index++] = ch;
  }

  return NULL;
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();

  while (1) {
    uint8_t cmd[] = "AT";
    uint8_t resp[128] = {0};
    if (NULL == AT_Command(&huart1, cmd, resp, sizeof(resp))) {
      UART_Printf("AT command failed!\n\r");
    } else {
      UART_Printf("Response: %s\n\r", resp);
    }
    HAL_Delay(1000);
  }
}
