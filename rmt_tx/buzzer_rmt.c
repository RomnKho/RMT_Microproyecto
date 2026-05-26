/**
 ******************************************************************************
 * @file    buzzer_rmt.c
 * @author  Luis Mario
 * @version V1.0
 * @date    2026-05-13
 * @brief   Driver RMT para buzzer pasivo ESP32
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "buzzer_rmt.h"

/* Private defines -----------------------------------------------------------*/
#define BUZZER_GPIO            GPIO_NUM_5
#define RMT_RESOLUTION_HZ      1000000UL
#define BUZZER_QUEUE_DEPTH     4
#define TONE_FREQ_HZ            2000
/* Private variables ---------------------------------------------------------*/
static const char *TAG = "BUZZER_RMT";
static rmt_encoder_handle_t copy_encoder = NULL;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Inicializa el canal RMT TX del buzzer
 */
rmt_channel_handle_t buzzer_rmt_init(void)
{
    rmt_channel_handle_t tx_chan = NULL;

    rmt_tx_channel_config_t tx_config = {
        .gpio_num = BUZZER_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = BUZZER_QUEUE_DEPTH,
        .flags.with_dma = false,
    };

    esp_err_t err;
    err = rmt_new_tx_channel(&tx_config, &tx_chan);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error creando canal TX");
        return NULL;
    }

    rmt_carrier_config_t carrier_config = {
        .frequency_hz = TONE_FREQ_HZ,
        .duty_cycle = 0.5f,         
        .flags.polarity_active_low = false 
    };

    err = rmt_apply_carrier(tx_chan, &carrier_config);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error aplicando portadora");
        return NULL;
    }

    rmt_copy_encoder_config_t encoder_config = {};
    err = rmt_new_copy_encoder(&encoder_config, &copy_encoder);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error creando encoder");
        return NULL;
    }

    err = rmt_enable(tx_chan);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error habilitando RMT");
        return NULL;
    }
    ESP_LOGI(TAG, "Buzzer inicializado GPIO %d", BUZZER_GPIO);
    return tx_chan;
}

/**
 * @brief Reproduce una nota musical
 */
void buzzer_rmt_play_tone(rmt_channel_handle_t tx_chan, uint32_t freq_hz, uint32_t duration_ms)
{
    if (tx_chan == NULL) 
    {
        return;
    }

    /* Silencio */
    if (freq_hz == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        return;
    }

    uint32_t period_us = 1000000UL / freq_hz;
    uint32_t half_period_us = period_us / 2;
    uint32_t cycles = (duration_ms * 1000UL) / period_us;
    /* Generar símbolos para un tono cuadrado */
    size_t symbol_count = cycles;

    rmt_symbol_word_t *symbols =
        malloc(symbol_count * sizeof(rmt_symbol_word_t));

    if (symbols == NULL)
    {
        ESP_LOGE(TAG, "Sin memoria para %lu Hz",
                 (unsigned long)freq_hz);
        return;
    }

    for (size_t i = 0; i < symbol_count; i++)
    {

        symbols[i] = (rmt_symbol_word_t) 
        {
            .level0 = 1,
            .duration0 = half_period_us,

            .level1 = 0,
            .duration1 = half_period_us,
        };
    }

    
    rmt_transmit_config_t transmit_config = {
        .loop_count = 0,
    };

    esp_err_t err;
    err = rmt_transmit(tx_chan, copy_encoder, symbols, symbol_count * sizeof(rmt_symbol_word_t), &transmit_config);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error transmitiendo tono");
        free(symbols);
        return;
    }

    /* Esperar a que termine la transmisión */
    err = rmt_tx_wait_all_done(tx_chan, portMAX_DELAY);

    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error esperando transmisión");
    }

    free(symbols);
}

/**
 * @brief Reproduce una melodía completa
 */
void buzzer_rmt_play_melody(rmt_channel_handle_t tx_chan, const buzzer_note_t *melody, size_t note_count)
{
    if (melody == NULL || note_count == 0) 
    {
        return;
    }

    for (size_t i = 0; i < note_count; i++)
    {
        buzzer_rmt_play_tone(tx_chan,melody[i].frequency, melody[i].duration_ms);
        /* Pequeña pausa entre notas */
        vTaskDelay(pdMS_TO_TICKS(melody[i].duration_ms / 10));
    }
}
