/**
 * @file    musical_buzzer.h
 * @brief   Header file for musical buzzer
 **/

#ifndef MUSICAL_BUZZER_H
#define MUSICAL_BUZZER_H

/** 
 * @brief Initilializes the TX channel and configures the encoder
 * @param[out] buzzer_chan => TX channel handler
 * @param[out] score_encoder => Encoder handler
 **/
void musical_buzzer_init(void);

/** 
 * @brief Plays the selected song
 * @param[in] buzzer_chan => TX channel handler
 * @param[in] score_encoder => Encoder handler
 * @param[in] song => Number of the wanted song
 **/
void musical_buzzer_play_song(uint8_t song);

#endif /* MUSICAL_BUZZER_H */

/* ======== END OF FILE ======== */