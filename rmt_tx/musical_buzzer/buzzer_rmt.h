/**
 ******************************************************************************
 * @file    buzzer_rmt.h
 * @author  Luis Mario
 * @version V1.0
 * @date    2026-05-13
 * @brief   Driver RMT para buzzer pasivo ESP32
 ******************************************************************************
 */

#ifndef BUZZER_RMT_H
#define BUZZER_RMT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "driver/rmt_tx.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Estructura de una nota musical
 */
typedef struct {
    uint32_t frequency;     /*!< Frecuencia en Hz */
    uint32_t duration_ms;   /*!< Duración en ms */
} buzzer_note_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Inicializa el periférico RMT para el buzzer.
 *
 * @return Handle del canal RMT o NULL si falla.
 */
rmt_channel_handle_t buzzer_rmt_init(void);

/**
 * @brief Reproduce una nota musical.
 *
 * @param tx_chan     Canal RMT.
 * @param freq_hz     Frecuencia en Hz.
 * @param duration_ms Duración en milisegundos.
 */
void buzzer_rmt_play_tone(rmt_channel_handle_t tx_chan,
                          uint32_t freq_hz,
                          uint32_t duration_ms);

/**
 * @brief Reproduce una melodía completa.
 *
 * @param tx_chan    Canal RMT.
 * @param melody     Array de notas.
 * @param note_count Número de notas.
 */
void buzzer_rmt_play_melody(rmt_channel_handle_t tx_chan,
                            const buzzer_note_t *melody,
                            size_t note_count);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_RMT_H */