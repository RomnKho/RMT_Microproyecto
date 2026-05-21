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

extern QueueHandle_t recv_queue;
extern rmt_channel_handle_t rx_channel;
extern led_strip_handle_t led_strip;

void app_main(void)
{
    control_remoto_init();
    xTaskCreate(tarea_control_remoto, "ir_task", 2048, NULL, 5, NULL);
    for(;;)
    {
        vTaskDelay(50 /portTICK_PERIOD_MS);
    }
}