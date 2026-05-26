/****** 
 * @file    wifi.h
 * @brief   Header file for wifi AP implementation
 ******/

#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

/***
 * @brief Prepares the wifi subsystem and connect to the AP
 ***/
void wifi_init(void);

/*** 
 * @brief Checks if the micro is connected
 * @return
 *      true  -> The micro is connected
 *      false -> The micro failed to connect
 ***/
bool wifi_is_connected(void);

#endif /* WIFI_H */

/* ===== END OF FILE ===== */