/***
 * @file    wifi.c
 * @brief   Wifi AP implementation 
 ***/

/* ===== INCLUDES ===== */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"

/* ===== DEFINES ===== */
#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/**** 
 * How the micro checks the password and ensures the connection (SAE / WPA3)
 * - Hunt and peck: Slower and vulnerable way to check the passowrd (loops) but works on old routers
 * - Hash to element (H2E): Faster and safer version to check the password (hash) but does not always work
 * - Both: Tries H2E when possible
 ****/ 

#if CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define H2E_IDENTIFIER ""

#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID

#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif

/**** 
 * How the micro chooses if the router is safe to connect:
 * - WIFI_AUTH_OPEN: Any router with / without safety protocol (public)
 * - WIFI_AUTH_WEP: Routers with WEP or better (Everything but public)
 * - WIFI_AUTH_WPA_PSK: WPA1 or better
 * - WIFI_AUTH_WPA2_PSK: WPA2, Mixed WPA2/WPA3 and WPA3
 * - WIFI_AUTH_WPA_WPA2_PSK: Mixed WPA1/WPA2 or better
 * - WIFI_AUTH_WPA3_PSK: Only WPA3
 * - WIFI_AUTH_WPA2_WPA3_PSK: Mixed WPA2/WPA3 and WPA3
 * - WIFI_AUTH_WAPI_PSK: Only for chinese safety protocol WAPI
 *
 * Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
 * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
 * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
 * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
 ****/

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN

#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP

#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK

#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK

#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK

#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK

#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK

#endif

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* ===== PRIVATE VARIABLES ===== */
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static TaskHandle_t wifi_check_task_handle = NULL;

static const char *TAG = "wifi.c";

static int s_retry_num = 0;
static volatile bool b_wifi_connected;

/* ===== PRIVATE EVENT HANDLER ===== */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if (s_retry_num < MAXIMUM_RETRY) 
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } 
        else 
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* ====== PRIVATE TASKS ===== */
static void wifi_check_task(void *pvParameters)
{
    EventBits_t bits;

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    bits = xEventGroupWaitBits(
            s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

    /* 
     * xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. 
     */
    if (bits & WIFI_CONNECTED_BIT) 
    {
        b_wifi_connected = true;
        ESP_LOGI(TAG, "connected to ap SSID:%s",
                 WIFI_SSID);
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        b_wifi_connected = false;
        ESP_LOGI(TAG, "Failed to connect to SSID:%s",
                 WIFI_SSID);
    } 
    else 
    {
        b_wifi_connected = false;
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    vTaskDelete(NULL);
}

/* ====== PRIVATE FUCNTIONS ===== */
static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

/* ====== EXPORTED FUNCIONS ===== */
void wifi_init(void)
{
    bool first_time = true;

    if (!first_time)
    {
        return;
    }

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    xTaskCreate(
        wifi_check_task,
        "wifi_check_task",
        2048,
        NULL,
        2,
        &wifi_check_task_handle);
}

bool wifi_is_connected(void)
{
    return b_wifi_connected;
}

/* ====== END OF FILE ====== */