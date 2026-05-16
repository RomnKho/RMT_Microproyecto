/**
 ******************************************************************************
 * @file    main.c
 * @brief   Ejemplo de uso del buzzer RMT
 ******************************************************************************
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buzzer_rmt.h"
#include "notas.h"

/*
 * Himno a la Alegría
 */
static const buzzer_note_t alegria_melody[] = {

    {NOTE_E4, 400},
    {NOTE_E4, 400},
    {NOTE_F4, 400},
    {NOTE_G4, 400},

    {NOTE_G4, 400},
    {NOTE_F4, 400},
    {NOTE_E4, 400},
    {NOTE_D4, 400},

    {NOTE_C4, 400},
    {NOTE_C4, 400},
    {NOTE_D4, 400},
    {NOTE_E4, 400},

    {NOTE_E4, 600},
    {NOTE_D4, 200},
    {NOTE_D4, 800},

    {NOTE_REST, 300},
};

void app_main(void)
{

    rmt_channel_handle_t rmt = buzzer_rmt_init();
    printf("RMT inicializado.\n");

    if (rmt == NULL) {

        printf("ERROR: No se pudo inicializar el buzzer\n");

        while (1) {
            vTaskDelay(portMAX_DELAY);
        }
    }

    size_t total_notes =sizeof(alegria_melody) / sizeof(buzzer_note_t);

    while (1) {

        printf("Reproduciendo melodía...\n");

        buzzer_rmt_play_melody(rmt, alegria_melody, total_notes
        );

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}