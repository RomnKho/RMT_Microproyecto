
/** 
 * @file    musical_buzzer.c 
 * @brief   Implementation of the musical buzzer
 **/

/* ===== INCLUDES =====*/
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "musical_score_encoder.h"
#include "driver/rmt_tx.h"
#include "musical_buzzer.h"

/* ===== DEFINES ===== */
#define RMT_BUZZER_RESOLUTION_HZ 1000000 // 1MHz resolution
#define RMT_BUZZER_GPIO_NUM      GPIO_NUM_27

/* ===== PRIVATE STRUCTS ===== */
typedef struct {
    const buzzer_musical_score_t *notes;
    size_t length;
} buzzer_song_t;

/* ===== PRIVATE VARIABLES ===== */
static const buzzer_musical_score_t ode_to_joy_score[] = {
    {740, 400}, {740, 600}, {784, 400}, {880, 400},
    {880, 400}, {784, 400}, {740, 400}, {659, 400},
    {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {740, 400}, {740, 200}, {659, 200}, {659, 800},

    {740, 400}, {740, 600}, {784, 400}, {880, 400},
    {880, 400}, {784, 400}, {740, 400}, {659, 400},
    {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {659, 400}, {659, 200}, {587, 200}, {587, 800},

    {659, 400}, {659, 400}, {740, 400}, {587, 400},
    {659, 400}, {740, 200}, {784, 200}, {740, 400}, {587, 400},
    {659, 400}, {740, 200}, {784, 200}, {740, 400}, {659, 400},
    {587, 400}, {659, 400}, {440, 400}, {440, 400},

    {740, 400}, {740, 600}, {784, 400}, {880, 400},
    {880, 400}, {784, 400}, {740, 400}, {659, 400},
    {587, 400}, {587, 400}, {659, 400}, {740, 400},
    {659, 400}, {659, 200}, {587, 200}, {587, 800},
};

static const buzzer_musical_score_t star_wars_score[] = {
    {392, 400}, {392, 400}, {392, 400}, 
    {523, 800}, {784, 800},
    {699, 200}, {659, 200}, {587, 200}, {1047, 800}, {784, 400},
    {699, 200}, {659, 200}, {587, 200}, {1047, 800}, {784, 400},
    {699, 200}, {659, 200}, {699, 200}, {587, 800}
};

static const buzzer_musical_score_t happy_birthday_score[] = {
    // Frase 1: Cum-ple-a-ños fe-liz
    {523, 200}, {523, 200}, {587, 400}, {523, 400}, {698, 400}, {659, 800},   

    // Frase 2: Cum-ple-a-ños fe-liz
    {523, 200}, {523, 200}, {587, 400}, {523, 400}, {784, 400}, {698, 800},   

    // Frase 3: Cum-ple-a-ños de "tu nom-bre"
    {523, 200}, {523, 200}, {1047, 400}, {880, 400}, {698, 400}, {659, 400}, {587, 800},   

    // Frase 4: Cum-ple-a-ños fe-liz
    {932, 200}, {932, 200}, {880, 400}, {698, 400}, {784, 400}, {698, 800}    
};

static const buzzer_song_t scores[] = {
    { ode_to_joy_score, sizeof(ode_to_joy_score) / sizeof(ode_to_joy_score[0])}, 
    {star_wars_score, sizeof(star_wars_score) / sizeof(star_wars_score[0])}, 
    {happy_birthday_score, sizeof(happy_birthday_score) / sizeof(happy_birthday_score[0])}
};

static rmt_channel_handle_t buzzer_chan;
static rmt_encoder_handle_t score_encoder;
static const char *TAG = "musical_buzzer.c";

/* ===== EXPORTED FUNCTIONS ===== */
void musical_buzzer_init(void)
{
    ESP_LOGI(TAG, "Create RMT TX channel");

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_BUZZER_GPIO_NUM,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_BUZZER_RESOLUTION_HZ,
        .trans_queue_depth = 10, // set the maximum number of transactions that can pend in the background
        .flags.with_dma = false
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &buzzer_chan));

    ESP_LOGI(TAG, "Install musical score encoder");
    musical_score_encoder_config_t encoder_config = {
        .resolution = RMT_BUZZER_RESOLUTION_HZ
    };
    ESP_ERROR_CHECK(rmt_new_musical_score_encoder(&encoder_config, &score_encoder));

}

void musical_buzzer_play_song(uint8_t song)
{

    ESP_ERROR_CHECK(rmt_enable(buzzer_chan));
    ESP_LOGI(TAG, "Playing song: %d", song);

    for (size_t i = 0; i < scores[song].length; i++) 
    {
        rmt_transmit_config_t tx_config = {
            .loop_count = -1
        };
        ESP_ERROR_CHECK(rmt_transmit(buzzer_chan, score_encoder, &scores[song].notes[i], sizeof(buzzer_musical_score_t), &tx_config));

        // Delay of the duration of the note
        vTaskDelay(pdMS_TO_TICKS(scores[song].notes[i].duration_ms));

        // Turn off and on to stop the infinite loop
        ESP_ERROR_CHECK(rmt_disable(buzzer_chan)); 
        ESP_ERROR_CHECK(rmt_enable(buzzer_chan));
    }

    ESP_ERROR_CHECK(rmt_disable(buzzer_chan));
}

/* ===== END OF FILE ===== */