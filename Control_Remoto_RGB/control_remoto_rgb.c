#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "driver/rmt_rx.h"
#include <led_strip.h>
#include "esp_log.h"
#include "control_remoto_rgb.h"

// GPIOS
#define RMT_RX_PIN  GPIO_NUM_14
#define LED_STRIP_PIN GPIO_NUM_21

#define LED_STRIP_NUM 8   // Cuántos LEDs tienes
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000) // Frecuencia del RMT

// Variables globales necesarias
QueueHandle_t recv_queue;
rmt_channel_handle_t rx_channel = NULL;
led_strip_handle_t led_strip = NULL;

static portMUX_TYPE control_lock = portMUX_INITIALIZER_UNLOCKED;

static volatile uint32_t red = 0,green = 0, blue = 0;
static const char *TAG = "ir_nec";


//Funcion para comprobar si una duración esta dentro del margen esperado
bool nec_check_in_range(uint32_t duration, uint32_t expected)
{
    return (duration < expected + 200) && (duration > expected - 200);
}

//Callback del control remoto: Se ejecuta en ISR y envia evento a la cola
bool rmt_rx_done_callback(rmt_channel_handle_t channel,const rmt_rx_done_event_data_t *edata,void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;  //Paraetro q indica si se ha despertado una tarea con mayor prioridad
    QueueHandle_t queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;  //Da paso a la tarea interrumpida
}

//Funcion para controlar los leds
void control_leds(uint16_t address, uint16_t command)
{
    uint32_t tmp_r = 0,tmp_g = 0,tmp_b = 0;
    switch (command)
    {
        //ON-OFF
        case 0xBA45:
            taskENTER_CRITICAL(&control_lock);
                red = 50;
                green = 50;
                blue = 50;
            taskEXIT_CRITICAL(&control_lock);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,5,5,5);
                led_strip_refresh(led_strip);            
            }
            vTaskDelay(250 /portTICK_PERIOD_MS);
            led_strip_clear(led_strip);
            vTaskDelay(250 /portTICK_PERIOD_MS);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,50,50,50);
                led_strip_refresh(led_strip);            
            }
            vTaskDelay(250 /portTICK_PERIOD_MS);
            led_strip_clear(led_strip);

            break;
        //0
        case 0xE916:
            led_strip_clear(led_strip);
            break;
        //1
        case 0xF30C:
            taskENTER_CRITICAL(&control_lock);
                red = 250;
                green = 0;
                blue = 0;
            taskEXIT_CRITICAL(&control_lock);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,250,0,0);
                led_strip_refresh(led_strip);            
            }            
            break;
        //2
        case 0xE718:
            taskENTER_CRITICAL(&control_lock);
                red = 0;
                green = 250;
                blue = 0;
            taskEXIT_CRITICAL(&control_lock);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,0,250,0);
                led_strip_refresh(led_strip);            
            }            
            break;
        //3
        case 0xA15E:
            taskENTER_CRITICAL(&control_lock);
                red = 0;
                green = 0;
                blue = 250;
            taskEXIT_CRITICAL(&control_lock);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,0,0,250);
                led_strip_refresh(led_strip);            
            }            
            break;
        //4
        case 0xF708:
            taskENTER_CRITICAL(&control_lock);
                red = 150;
                green = 100;
                blue = 0;
            taskEXIT_CRITICAL(&control_lock);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,150,100,0);
                led_strip_refresh(led_strip);

                vTaskDelay(500 /portTICK_PERIOD_MS);

                led_strip_set_pixel(led_strip,i,0,0,0);
                led_strip_refresh(led_strip);            
            }            
            break;
        //5
        case 0xE31C:
            taskENTER_CRITICAL(&control_lock);
                red = 150;
                green = 0;
                blue = 100;
            taskEXIT_CRITICAL(&control_lock);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,150,0,100);
                led_strip_refresh(led_strip);

                vTaskDelay(1000 /portTICK_PERIOD_MS);
                
                led_strip_set_pixel(led_strip,i,0,0,0);
                led_strip_refresh(led_strip);            
            }            
            break;
        //6
        case 0xA55A:
            taskENTER_CRITICAL(&control_lock);
                red = 50;
                green = 50;
                blue = 50;
            taskEXIT_CRITICAL(&control_lock);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,50,50,50);
                led_strip_refresh(led_strip);            
            }            
            break;
        //7
        case 0xBD42: 
            for (int i = 0; i < (LED_STRIP_NUM/2); i++)
            {
                led_strip_set_pixel(led_strip,i,150,100,0);
                led_strip_set_pixel(led_strip,(LED_STRIP_NUM-1)-i,0,150,100);            
            }   
            led_strip_refresh(led_strip);         
            break;
        //8
        case 0xAD52:
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,150,100,0);
                led_strip_refresh(led_strip);            
            }            
            break;
        //9
        case 0xB54A:
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,150,100,0);
                led_strip_refresh(led_strip);            
            }            
            break;
        //play  
        case 0xEA15:
            taskENTER_CRITICAL(&control_lock);
                    tmp_r = red;
                    tmp_g = green;
                    tmp_b = blue;
            taskEXIT_CRITICAL(&control_lock);
            for (int i = 0; i < LED_STRIP_NUM; i++)
            {
                led_strip_set_pixel(led_strip,i,tmp_r,tmp_g,tmp_b);
                led_strip_refresh(led_strip);

                vTaskDelay(500 /portTICK_PERIOD_MS);

                led_strip_set_pixel(led_strip,i,0,0,0);
                led_strip_refresh(led_strip);
            }            
            break;

        // más casos...
        default:
        
            break;
    }
}

//Funcion que descodifica la direccion y el comando
void tarea_control_remoto(void *pvParameters)
{
    rmt_rx_done_event_data_t rx_data;
    while (1) {
        if (xQueueReceive(recv_queue, &rx_data, portMAX_DELAY) == pdPASS) {
            rmt_symbol_word_t *tmp = rx_data.received_symbols;
            size_t num = rx_data.num_symbols;

            if (num >= 34 && nec_check_in_range(tmp->duration0, 9000) && 
                nec_check_in_range(tmp->duration1, 4500))
            {
                tmp++;
                uint16_t address = 0, command = 0;

                // Leer 16 bits de dirección (original + invertido)
                for (int i = 0; i < 16; i++) {
                    //Comprueba si es un 1 debido a que su espacio es de 1690
                    if (nec_check_in_range(tmp->duration0, 560) && 
                        nec_check_in_range(tmp->duration1, 1690))
                        address |= (1 << i);
                    tmp++;
                }
                // Leer 16 bits de comando (original + invertido)
                for (int i = 0; i < 16; i++) {
                    if (nec_check_in_range(tmp->duration0, 560) && 
                        nec_check_in_range(tmp->duration1, 1690))
                        command |= (1 << i);
                    tmp++;
                }

                // Compruba si hay errores de lectura por ruido
                // El byte bajo es el valor original, el byte alto es el invertido
                uint8_t addr_orig  = address & 0xFF;
                uint8_t addr_inv   = (address >> 8) & 0xFF;
                uint8_t cmd_orig   = command & 0xFF;
                uint8_t cmd_inv    = (command >> 8) & 0xFF;

                bool addr_ok = (addr_inv == (uint8_t)~addr_orig);
                bool cmd_ok  = (cmd_inv  == (uint8_t)~cmd_orig);

                if (addr_ok && cmd_ok) {
                    // Trama íntegra: procedemos como de costumbre
                    ESP_LOGI(TAG, "Address: 0x%04X, Command: 0x%04X", address, command);
                    control_leds(address, command);
                } else {
                    // Trama corrupta: la ignoramos (podemos registrar un aviso)
                    ESP_LOGW(TAG, "Trama NEC con error de integridad – descartada");
                }
                // -----------------------------------------
            }

            // Rearmar la recepción para la siguiente trama
            static rmt_symbol_word_t raw_symbols[64];
            rmt_receive_config_t recv_cfg = {
                .signal_range_min_ns = 1250,
                .signal_range_max_ns = 12000000,
            };
            rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &recv_cfg);
        }
    }
}
//Funcion para inicializar el hardware
void control_remoto_init(void)
{
    // Configuración de la tira de LEDs
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_PIN,
        .max_leds = LED_STRIP_NUM,
        .led_model = LED_MODEL_WS2812, // Determina los tiempos de los pulsos
    };

    // Configura el periferico para generar la señal de los LEDS
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,  //10MHz permite generar pulsos con precisión de 0,1 microseg
    };

    // Crea la tira de LEDs, combina la configuracion de la tira y la del rmt y la guarda en el manejador led_strip
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    // Configurar canal RX del RMT
    rmt_rx_channel_config_t rx_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1 * 1000 * 1000,   //1 µs por tick
        .mem_block_symbols = 64,            //Reserva memoria para almacenar hasya 64 simbolos RMT
        .gpio_num = RMT_RX_PIN,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_cfg, &rx_channel));  //Crea el canal de recepcion RMT

    // Crear cola para pasar eventos desde ISR
    recv_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    assert(recv_queue); //Detiene el programa si xQueueCreate devuelve NULL(fallo de memoria)

    // Registrar callback
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_done_callback,
    };
    //Asocia los callbacks al canal rx,
    //le pasa la direccion de memoria y 
    //pasa recv_queue como parametro para q reciba el callback y 
    //permita enviar el evento a la cola
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, recv_queue));

    // Habilita el canal y empeza a recibir
    ESP_ERROR_CHECK(rmt_enable(rx_channel));

    //Define los filtros de señal
    rmt_receive_config_t recv_cfg = {
        .signal_range_min_ns = 1250,        //ignora pulsos menores de 1,25 microseg (evita rebotes, ruidos)
        .signal_range_max_ns = 12000000,    //ignora pulsos mayores a 12ms (evita falsos finales)
    };
    static rmt_symbol_word_t raw_symbols[64];   //Reserva un buffer de 64 simbolos para capturar la señal
    //inicia la recepcion en segundo plano
    ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &recv_cfg));

    ESP_LOGI(TAG, "Receptor IR listo en GPIO %d", RMT_RX_PIN);
}