/**
    Copyright (C) 2025 The Sistemas Empotrados subject at UPV
    
    @file    control_remoto_rgb.c
    @author  David Pérez
    @version V0.4
    @date    2025-02-27
    @brief   Implementation of IR remote control and RGB LED module.
          
    Contains NEC protocol decoding logic, WS2812 LED strip handling,
    and RMT peripheral initialization functions.
*/

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "driver/rmt_rx.h"
#include <led_strip.h>
#include "esp_log.h"
#include "control_remoto_rgb.h"

/* Private typedef -----------------------------------------------------------*/
/* No private types */

/* Private define ------------------------------------------------------------*/
#define RMT_RX_PIN          GPIO_NUM_14
#define LED_STRIP_PIN       GPIO_NUM_21
#define LED_STRIP_NUM       8
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

/* Private macro -------------------------------------------------------------*/
/* No private macros */

/* Private variables ---------------------------------------------------------*/
static QueueHandle_t recv_queue;
static rmt_channel_handle_t rx_channel = NULL;
static led_strip_handle_t led_strip = NULL;
static portMUX_TYPE control_lock = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t red = 0, green = 0, blue = 0;
static const char *TAG = "ir_nec";

/* Private function prototypes -----------------------------------------------*/
/* No private functions; all static functions are implicitly declared */

/* Exported functions --------------------------------------------------------*/
/******************************************************************************/
/**
    @brief  Checks if a measured duration is within the expected range.
    @param  duration Measured duration (in microseconds).
    @param  expected Expected duration (in microseconds).
    @retval true if within range, false otherwise.
*/
bool nec_check_in_range(uint32_t duration, uint32_t expected)
{
    return (duration < expected + 200) && (duration > expected - 200);
}

/******************************************************************************/
/**
    @brief  Remote control callback. Runs in ISR and sends event to queue.
    @param  channel RMT channel that finished reception.
    @param  edata   Reception done event data.
    @param  user_data Pointer to the queue where the event will be sent.
    @retval true if a task switch is required, false otherwise.
*/
bool rmt_rx_done_callback(rmt_channel_handle_t channel,
                          const rmt_rx_done_event_data_t *edata,
                          void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

/******************************************************************************/
/**
    @brief  Controls the LED strip according to the received address and command.
    @param  address Remote address.
    @param  command Command code (pressed key).
    @retval None.
*/
void leds_control(uint16_t address, uint16_t command)
{
    uint32_t tmp_r = 0, tmp_g = 0, tmp_b = 0;
    if(address == 0xFF00) {
        switch (command)
        {
            // ON-OFF
            case 0xBA45:
                taskENTER_CRITICAL(&control_lock);
                    red = 50;
                    green = 50;
                    blue = 50;
                taskEXIT_CRITICAL(&control_lock);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 5, 5, 5);
                    led_strip_refresh(led_strip);
                }
                vTaskDelay(250 / portTICK_PERIOD_MS);
                led_strip_clear(led_strip);
                vTaskDelay(250 / portTICK_PERIOD_MS);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 50, 50, 50);
                    led_strip_refresh(led_strip);
                }
                vTaskDelay(250 / portTICK_PERIOD_MS);
                led_strip_clear(led_strip);
                break;
            // 0
            case 0xE916:
                led_strip_clear(led_strip);
                break;
            // 1
            case 0xF30C:
                taskENTER_CRITICAL(&control_lock);
                    red = 250;
                    green = 0;
                    blue = 0;
                taskEXIT_CRITICAL(&control_lock);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 250, 0, 0);
                    led_strip_refresh(led_strip);
                }
                break;
            // 2
            case 0xE718:
                taskENTER_CRITICAL(&control_lock);
                    red = 0;
                    green = 250;
                    blue = 0;
                taskEXIT_CRITICAL(&control_lock);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 0, 250, 0);
                    led_strip_refresh(led_strip);
                }
                break;
            // 3
            case 0xA15E:
                taskENTER_CRITICAL(&control_lock);
                    red = 0;
                    green = 0;
                    blue = 250;
                taskEXIT_CRITICAL(&control_lock);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 0, 0, 250);
                    led_strip_refresh(led_strip);
                }
                break;
            // 4
            case 0xF708:
                taskENTER_CRITICAL(&control_lock);
                    red = 150;
                    green = 100;
                    blue = 0;
                taskEXIT_CRITICAL(&control_lock);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 150, 100, 0);
                    led_strip_refresh(led_strip);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    led_strip_set_pixel(led_strip, i, 0, 0, 0);
                    led_strip_refresh(led_strip);
                }
                break;
            // 5
            case 0xE31C:
                taskENTER_CRITICAL(&control_lock);
                    red = 150;
                    green = 0;
                    blue = 100;
                taskEXIT_CRITICAL(&control_lock);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 150, 0, 100);
                    led_strip_refresh(led_strip);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    led_strip_set_pixel(led_strip, i, 0, 0, 0);
                    led_strip_refresh(led_strip);
                }
                break;
            // 6
            case 0xA55A:
                taskENTER_CRITICAL(&control_lock);
                    red = 50;
                    green = 50;
                    blue = 50;
                taskEXIT_CRITICAL(&control_lock);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 50, 50, 50);
                    led_strip_refresh(led_strip);
                }
                break;
            // 7
            case 0xBD42:
                for (int i = 0; i < (LED_STRIP_NUM / 2); i++)
                {
                    led_strip_set_pixel(led_strip, i, 150, 100, 0);
                    led_strip_set_pixel(led_strip, (LED_STRIP_NUM - 1) - i, 0, 150, 100);
                }
                led_strip_refresh(led_strip);
                break;
            // 8
            case 0xAD52:
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 150, 100, 0);
                    led_strip_refresh(led_strip);
                }
                break;
            // 9
            case 0xB54A:
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, 150, 100, 0);
                    led_strip_refresh(led_strip);
                }
                break;
            // play
            case 0xEA15:
                taskENTER_CRITICAL(&control_lock);
                    tmp_r = red;
                    tmp_g = green;
                    tmp_b = blue;
                taskEXIT_CRITICAL(&control_lock);
                for (int i = 0; i < LED_STRIP_NUM; i++)
                {
                    led_strip_set_pixel(led_strip, i, tmp_r, tmp_g, tmp_b);
                    led_strip_refresh(led_strip);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    led_strip_set_pixel(led_strip, i, 0, 0, 0);
                    led_strip_refresh(led_strip);
                }
                break;
            default:
                break;
        }
    }
    else 
    {
        ESP_LOGW(TAG, "Received command for unknown address: 0x%04X", address);
    }
    
}

/******************************************************************************/
/**
    @brief  FreeRTOS task that decodes NEC frames and calls control_leds.
    @param  pvParameters Unused parameter.
    @retval None (never returns).
*/
void remote_control_task(void *pvParameters)
{
    rmt_rx_done_event_data_t rx_data;
    while (1) {
        if (xQueueReceive(recv_queue, &rx_data, portMAX_DELAY) == pdPASS) {
            rmt_symbol_word_t *tmp = rx_data.received_symbols;
            size_t num = rx_data.num_symbols;

            if (num >= 34 && nec_check_in_range(tmp->duration0, 9000) && 
                nec_check_in_range(tmp->duration1, 4500))
            {
                tmp++;
                uint16_t address = 0, command = 0;

                // Read 16 address bits (original + inverted)
                for (int i = 0; i < 16; i++) {
                    if (nec_check_in_range(tmp->duration0, 560) && 
                        nec_check_in_range(tmp->duration1, 1690))
                        address |= (1 << i);
                    tmp++;
                }
                // Read 16 command bits (original + inverted)
                for (int i = 0; i < 16; i++) {
                    if (nec_check_in_range(tmp->duration0, 560) && 
                        nec_check_in_range(tmp->duration1, 1690))
                        command |= (1 << i);
                    tmp++;
                }

                // Check integrity (original vs inverted)
                uint8_t addr_orig = address & 0xFF;
                uint8_t addr_inv  = (address >> 8) & 0xFF;
                uint8_t cmd_orig  = command & 0xFF;
                uint8_t cmd_inv   = (command >> 8) & 0xFF;

                bool addr_ok = (addr_orig == (uint8_t)(~addr_inv));
                bool cmd_ok  = (cmd_orig  == (uint8_t)(~cmd_inv));

                if (addr_ok && cmd_ok) {
                    ESP_LOGI(TAG, "Address: 0x%04X, Command: 0x%04X", address, command);
                    leds_control(address, command);
                } else {
                    ESP_LOGW(TAG, "NEC frame with integrity error discarded");
                }
            }

            // Re-arm reception for the next frame
            static rmt_symbol_word_t raw_symbols[64];
            rmt_receive_config_t recv_cfg = {
                .signal_range_min_ns = 1250,
                .signal_range_max_ns = 12000000,
            };
            rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &recv_cfg);
        }
    }
}

/******************************************************************************/
/**
    @brief  Initializes hardware: LED strip, RMT RX channel, queue and callbacks.
    @retval None.
*/
void remote_control_init(void)
{
    // LED strip configuration
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_PIN,
        .max_leds = LED_STRIP_NUM,
        .led_model = LED_MODEL_WS2812,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    // Configure RMT RX channel
    rmt_rx_channel_config_t rx_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1 * 1000 * 1000,   // 1 µs per tick
        .mem_block_symbols = 64,
        .gpio_num = RMT_RX_PIN,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_cfg, &rx_channel));

    // Create queue to pass events from ISR
    recv_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    assert(recv_queue);

    // Register callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_done_callback,
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, recv_queue));

    // Enable channel and start reception
    ESP_ERROR_CHECK(rmt_enable(rx_channel));

    // Set signal filters
    rmt_receive_config_t recv_cfg = {
        .signal_range_min_ns = 1250,
        .signal_range_max_ns = 12000000,
    };
    static rmt_symbol_word_t raw_symbols[64];
    ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &recv_cfg));

    ESP_LOGI(TAG, "IR receiver ready on GPIO %d", RMT_RX_PIN);
}

/* Private functions ---------------------------------------------------------*/
/* No private functions in this module */

/* End of file ****************************************************************/
