/**
    Copyright (C) 2025 The Sistemas Empotrados subject at UPV
    
    @file    main.c
    @author  David Pérez
    @version V0.4
    @date    2025-02-27
    @brief   Main program for IR remote control and RGB LED strip.
          
    Initializes the remote control module, creates the decoding task,
    and enters an infinite loop to keep the system running.
*/

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/rmt_rx.h"
#include <led_strip.h>
#include "control_remoto_rgb.h"
#include "esp_log.h"

/* Private typedef -----------------------------------------------------------*/
/* No private types */

/* Private define ------------------------------------------------------------*/
/* No private macros */

/* Private macro -------------------------------------------------------------*/
/* No private macros */

/* Private variables ---------------------------------------------------------*/
/* External variables declared in control_remoto_rgb.h are referenced but not defined here */

/* Private function prototypes -----------------------------------------------*/
/* No private function prototypes */

/* Exported functions --------------------------------------------------------*/
/******************************************************************************/
/**
    @brief  Application entry point.
    @retval None (never returns).
*/
void app_main(void)
{
    remote_control_init();
    xTaskCreate(remote_control_task, "ir_task", 2048, NULL, 5, NULL);
    for (;;)
    {
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

/* Private functions ---------------------------------------------------------*/
/* No private functions */

/* End of file ****************************************************************/