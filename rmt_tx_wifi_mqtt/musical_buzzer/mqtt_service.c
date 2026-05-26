/**
 * mqtt-service.h 
 * Deriver of mqtt tcp example from official expressif
*/

/* ===== INCLUDES ===== */
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_service.h"
#include "freertos/queue.h"

#include "json_parser.h"

/* ====== PRIVATE VARIABLES ====== */
static const char *TAG = "mqtt_service.c";
static const char *PUB_TOPIC = "emqx/SEM/pub";

static esp_mqtt_client_handle_t client;
static bool volatile    mqtt_service_connected = false;

static const uint8_t    RECEIVED_DATA_QUEUE_SIZE = 10;
QueueHandle_t           received_data_mqtt_queue;

static int              song_number_json;
static uint8_t          song_number;
static jparse_ctx_t     json_parse;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/* ====== EXPORTED FUNCTION ====== */
/****
 * @todo: Implementar cola para mandar datos a main
 * @todo: Procesamiento de datos
 * @todo: Más canciones para elegir en app
 * @todo: app con Node-Red
 ****/ 
void mqtt_service_init(const char* uri)
{
    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .broker.address.uri = uri,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    /* INICIALIZAR COLA DE DATA */
    received_data_mqtt_queue = xQueueCreate(RECEIVED_DATA_QUEUE_SIZE, sizeof(uint8_t));
}

int mqtt_service_publish(const char *topic, const char *data, int len)
{
    // qos = 0 => The sender transmits the message exactly once and does not wait for an acknowledgment
    // qos = 1 => The sender transmits the message and holds onto it until it receives an acknowledgment (PUBACK) from the receiver.
    // qos = 2 => It utilizes a four-step handshake to ensure the message is delivered and processed exactly once without any duplicates. 
    return esp_mqtt_client_publish(client, topic, data, len, 0, 0); 
}

int mqtt_service_subscribe(const char *topic)
{
    return esp_mqtt_client_subscribe_single(client, topic, 0);
}

bool mqtt_service_is_connected(void)
{
    return mqtt_service_connected;
}

/* ====== PRIVATE FUNCTIONS ====== */
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;    
    // int msg_id;
    client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        mqtt_service_connected = true;
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_publish(client, PUB_TOPIC, "Ready from Roman's ESP32", 0, 0, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        mqtt_service_connected = false;
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "Received data from topic = %.*s", event->topic_len, event->topic);

        json_parse_start(&json_parse, event->data, event->data_len);
        if (json_obj_get_int(&json_parse, "song_number", &song_number_json) != ESP_OK)
        {
            ESP_LOGE(TAG, "MQTT received data song_number is NOT an int");
        }
        else
        {
            song_number = (uint8_t)song_number_json;

            if (song_number >= 0 || song_number <= 2)
            {
                if (xQueueSend(received_data_mqtt_queue, &song_number, 0) != pdPASS)
                {
                    ESP_LOGW(TAG, "MQTT queue is full");
                }
            }
            else
            {
                ESP_LOGW(TAG, "Only songs 0-2");
            }
        }

        json_parse_end(&json_parse);

        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            // log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            // log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            // log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/*** End of file ***/
