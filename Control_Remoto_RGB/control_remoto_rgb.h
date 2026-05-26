/**
    Copyright (C) 2025 The Sistemas Empotrados subject at UPV
    
    @file    control_remoto_rgb.h
    @author  David Pérez
    @version V0.4
    @date    2025-02-27
    @brief   Module for IR remote control (NEC protocol) and RGB LED strip management.
          
    This module initializes the IR receiver using RMT, decodes NEC frames,
    and controls a WS2812 LED strip based on received commands.
    Public functions are intended to be used from main.
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CONTROL_REMOTO_RGB_H
#define CONTROL_REMOTO_RGB_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "driver/rmt_rx.h"

/* Exported types ------------------------------------------------------------*/
/* No specific types are defined in this module */

/* Exported constants --------------------------------------------------------*/
/* No exported constants */

/* Exported macro ------------------------------------------------------------*/
/* No exported macros */

/* Exported functions --------------------------------------------------------*/ 
/**
 * @brief  Checks if a measured duration is within the expected range (tolerance ±200 µs).
 * @param  duration Measured duration (in microseconds).
 * @param  expected Expected duration (in microseconds).
 * @retval true if within range, false otherwise.
 */
bool nec_check_in_range(uint32_t duration, uint32_t expected);

/**
 * @brief  Remote control callback. Runs in ISR context and sends an event to the queue.
 * @param  channel RMT channel that finished reception.
 * @param  edata   Reception done event data.
 * @param  user_data Pointer to the queue (QueueHandle_t) where the event will be sent.
 * @retval true if a task switch is required, false otherwise.
 */
bool rmt_rx_done_callback(rmt_channel_handle_t channel,
                          const rmt_rx_done_event_data_t *edata,
                          void *user_data);

/**
 * @brief  Controls the LED strip according to the received address and command.
 * @param  address Remote address (typically 0x00FF for standard NEC).
 * @param  command Command code (pressed key).
 * @retval None.
 */
void control_leds(uint16_t address, uint16_t command);

/**
 * @brief  FreeRTOS task that decodes NEC frames and calls control_leds.
 * @param  pvParameters Unused parameter.
 * @retval None (never returns).
 */
void tarea_control_remoto(void *pvParameters);

/**
 * @brief  Initializes hardware: LED strip, RMT RX channel, queue and callbacks.
 * @retval None.
 */
void control_remoto_init(void);

#ifdef __cplusplus
}
#endif

#endif
/*** End of file **************************************************************/