/*********************************************************** 
 * @file    main.c                                         *
 * @brief   Testing how RMT TX works with a passive buzzer *
 ***********************************************************/

/* ===== INCLUDES ===== */
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "wifi.h"
#include "mqtt_service.h"
#include "musical_buzzer.h"

/* ====== DEFINES ====== */
#define     MQTT_INIT_DELAY     2000
#define     MAIN_DELAY          1000

/* ===== PRIVATE VARIABLES ===== */

static const char *BROKER    = "mqtt://broker.emqx.io";
static const char *SUB_TOPIC = "emqx/SEM/sub";

static uint8_t song_number;

/* ===== MAIN ===== */
void app_main(void)
{
    wifi_init();

    while(!wifi_is_connected())
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    mqtt_service_init(BROKER);
    vTaskDelay(pdMS_TO_TICKS(MQTT_INIT_DELAY));

    mqtt_service_subscribe(SUB_TOPIC);
    vTaskDelay(pdMS_TO_TICKS(MQTT_INIT_DELAY));

    musical_buzzer_init();

    for(;;)
    {
        xQueueReceive(received_data_mqtt_queue, &song_number, portMAX_DELAY);
        musical_buzzer_play_song(song_number);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ===== END OF FILE ===== */