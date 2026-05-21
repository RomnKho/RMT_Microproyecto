#ifndef CONTROL_REMOTO_RGB_H
#define CONTROL_REMOTO_RGB_H

#include <stdint.h>
#include "driver/rmt_rx.h"

//Funcion para comprobar si una duración esta dentro del margen esperado
bool nec_check_in_range(uint32_t duration, uint32_t expected);

//Callback del control remoto: Se ejecuta en ISR y envia evento a la cola
bool rmt_rx_done_callback(rmt_channel_handle_t channel,const rmt_rx_done_event_data_t *edata,void *user_data);

//Funcion paraontrolar los leds
void control_leds(uint16_t address, uint16_t command);

//Funcion que descodifica la direccion y el comando
void tarea_control_remoto(void *pvParameters);

//Funcion para inicializar el hardware
void control_remoto_init(void);

#endif