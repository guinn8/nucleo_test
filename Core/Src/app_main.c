/**
 * @file app_main.c
 * @brief Application code built on the Nucleo-F411RE board and STM32Cube autogenerated code.
 * @date 2024-06-27
 */

#include <stdio.h>
#include "epd_driver.h"

int app_main(void) {
    printf("\n\r(%s:%d) START PROGRAM\n\r", __FILE__, __LINE__);

    epd_initialize();

        setup_network();
        server_communication();

    while (1) {

        HAL_Delay(1000);
    }

    return 0;
}
