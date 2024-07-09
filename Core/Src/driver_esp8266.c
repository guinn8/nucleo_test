/**
 * @file driver_esp8266.c
 * @author Gavin Guinn
 * @brief Functions to control an ESP8266 over a uart interface.
 * @date 2024-06-28
 *
 */

#include "driver_esp8266.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "DEV_Config.h"
#include "EPD_2in13_V3.h"
#include "GUI_Paint.h"
#include "driver_esp8266.h"
#include "epd_driver.h"
#include "main.h"

extern UART_HandleTypeDef huart1;
static UART_HandleTypeDef *modem_uart = &huart1;
static char resp[1024] = {0};
static uint16_t read_index = 0;
volatile uint16_t write_index = 0;
static uint16_t start_at_cmd = 0;

void copy_string_from_circular_buffer(char *response, size_t response_len) {
  size_t len =
      (write_index >= start_at_cmd) ? (write_index - start_at_cmd) : (sizeof(resp) - start_at_cmd + write_index);
  len = (len > response_len - 1) ? (response_len - 1) : len;
  strncpy(response, &resp[start_at_cmd], len);
  response[len] = '\0';
}

bool send_at_command_and_check_response(char *cmd, char *expected_response, char *response, size_t response_len) {
  HAL_UART_Transmit(modem_uart, (uint8_t *)cmd, strlen(cmd), HAL_MAX_DELAY);

  uint32_t start_tick = HAL_GetTick();
  start_at_cmd = read_index;
  while ((HAL_GetTick() - start_tick) < 20000) {
    write_index = (sizeof(resp) - __HAL_DMA_GET_COUNTER(modem_uart->hdmarx)) % sizeof(resp);
    while (read_index != write_index) {
      putchar(resp[read_index]);
      if (strncmp(&resp[read_index], expected_response, strlen(expected_response)) == 0) {
	printf("Received %s for %s\r\n", expected_response, cmd);
	read_index += strlen(expected_response);
	copy_string_from_circular_buffer(response, response_len);
	return true;
      }
      read_index = (read_index + 1) % sizeof(resp);
    }
    HAL_Delay(10);
  }

  printf("Command %s failed!\r\n", cmd);
  return false;
}



void server_communication(void) {
  char response[1024] = {0};
  if (send_at_command_and_check_response("AT+CIPSERVER=1\r\n", "CONNECT", response, sizeof(response))) {
    printf("Server created on port 333.\n\r");
  }

  if (send_at_command_and_check_response("+IPD, 0:\r\n", "+IPD, 0:", response, sizeof(response))) {
    char *num_chars_start = strstr(response, ",0,");
    if (num_chars_start) {
      num_chars_start += 3; // Move past ",0,"
      int num_chars = atoi(num_chars_start);
      

      // Find the start of the message
      char *msg_start = strstr(num_chars_start, ":");
      if (msg_start) {
        msg_start++; // Move past ":"
        char message[1024] = {0};
        strncpy(message, msg_start, num_chars);
        message[num_chars] = '\0'; // Ensure null-termination
        epd_display_line(0, 1, message);
      }
    }
  }



  
}


void setup_network(void) {
  HAL_UART_Receive_DMA(modem_uart, (uint8_t *)resp, sizeof(resp));

  char response[1024] = {0};
  if (send_at_command_and_check_response("AT+RST\r\n", "ready", response, sizeof(response))) {
    printf("Module reset.\n\r");
  }

  if (send_at_command_and_check_response("AT\r\n", "OK", response, sizeof(response))) {
    printf("AT OK.\n\r");
  }

  if (send_at_command_and_check_response("AT+CWMODE_CUR=3\r\n", "OK", response, sizeof(response))) {
    printf("WiFi mode set.\n\r");
  }

  if (send_at_command_and_check_response("AT+CWJAP_CUR=\"tiglath\",\"thedog123\"\r\n", "OK", response,
					 sizeof(response))) {
    printf("Connected to WiFi.\n\r");
  }

  if (send_at_command_and_check_response("AT+CIFSR\r\n", "OK", response, sizeof(response))) {
    printf("Got IP address.\n\r");
  }

  if (send_at_command_and_check_response("AT+CWMODE=3\r\n", "OK", response, sizeof(response))) {
    printf("CIPMODE 1 mode set.\n\r");
  }

  if (send_at_command_and_check_response("AT+CIPMUX=1\r\n", "OK", response, sizeof(response))) {
    printf("CIPMODE 1 mode set.\n\r");
  }


}
