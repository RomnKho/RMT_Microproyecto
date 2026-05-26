/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_check.h"
#include "musical_score_encoder.h"

static const char *TAG = "score_encoder";

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *copy_encoder; // copy_encoder translates into binary
    uint32_t resolution;
} rmt_musical_score_encoder_t;


/* ================== PRIVATE FUNCTIONS ================== */

/********************************
 * @brief Configs the note encoder - Callback 
 *******************************/
RMT_ENCODER_FUNC_ATTR
static size_t rmt_encode_musical_score(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    // __containerof gives where the structure starts in memory
    // We're changing the espressif encoder structure with ours (rmt_musical_score_encoder_t)
    // We're telling the compiler that 'base' is where the structure starts in memory
    rmt_musical_score_encoder_t *score_encoder = __containerof(encoder, rmt_musical_score_encoder_t, base);
    rmt_encoder_handle_t copy_encoder = score_encoder->copy_encoder;

    // RMT_ENCODING_RESET cleans the previous encoded note and is ready to start the config again
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;

    // primary_data has the note_info (freq_hz | duration_ms)
    buzzer_musical_score_t *score = (buzzer_musical_score_t *)primary_data;

    // Config the rmt signal to be squared
    uint32_t rmt_raw_symbol_duration = score_encoder->resolution / score->freq_hz / 2;
    rmt_symbol_word_t musical_score_rmt_symbol = 
    {
        .level0 = 0,
        .duration0 = rmt_raw_symbol_duration,
        .level1 = 1,
        .duration1 = rmt_raw_symbol_duration,
    };

    size_t encoded_symbols = copy_encoder->encode(copy_encoder, channel, &musical_score_rmt_symbol, sizeof(musical_score_rmt_symbol), &session_state);
    // After this, session_state will change to RMT_ENCODING_COMPLETE which means that the encoding has nothing else to do
    // or RMT_ENCODING_INCOMPLETE which means that the encoding had problems to deal with
    *ret_state = session_state;
    return encoded_symbols;
}

/********************************************************
 * @brief Configs the delete encoder funcion - Callback
 ********************************************************/
static esp_err_t rmt_del_musical_score_encoder(rmt_encoder_t *encoder)
{
    rmt_musical_score_encoder_t *score_encoder = __containerof(encoder, rmt_musical_score_encoder_t, base);
    rmt_del_encoder(score_encoder->copy_encoder);
    free(score_encoder);
    return ESP_OK;
}

/********************************************************
 * @brief Configs the reset encoder funcion - Callback
 ********************************************************/
RMT_ENCODER_FUNC_ATTR
static esp_err_t rmt_musical_score_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_musical_score_encoder_t *score_encoder = __containerof(encoder, rmt_musical_score_encoder_t, base);
    rmt_encoder_reset(score_encoder->copy_encoder);
    return ESP_OK;
}

/* ================== EXPORTED FUNCTIONS ================== */

esp_err_t rmt_new_musical_score_encoder(const musical_score_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_musical_score_encoder_t *score_encoder = NULL;
    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");

    // Allocates a proper memory block for RMT encoder
    score_encoder = rmt_alloc_encoder_mem(sizeof(rmt_musical_score_encoder_t));
    ESP_GOTO_ON_FALSE(score_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for musical score encoder");

    // Defines the callbacks
    score_encoder->base.encode = rmt_encode_musical_score;
    score_encoder->base.del = rmt_del_musical_score_encoder;
    score_encoder->base.reset = rmt_musical_score_encoder_reset;

    // Resolution used by user
    score_encoder->resolution = config->resolution;
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &score_encoder->copy_encoder), err, TAG, "create copy encoder failed");

    // Return the encoder handler
    *ret_encoder = &score_encoder->base;
    return ESP_OK;

    // If there was an error, the encoder will delete itself 
err:
    if (score_encoder) {
        if (score_encoder->copy_encoder) {
            rmt_del_encoder(score_encoder->copy_encoder);
        }
        free(score_encoder);
    }
    return ret;
}

/* ================== END OF FILE ================== */