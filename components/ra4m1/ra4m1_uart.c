#include <stdlib.h>

#include "esp_log.h"
#include "driver/uart.h"

#include "ra4m1_ctrl.h"
#include "ra4m1_uart.h"

#define RA4M1_UART_BAUD_RATE 115200
#define RA4M1_UART_BUFFER_SIZE (1024 * 2)
#define RA4M1_UART_EVENT_QUEUE_SIZE 16

typedef struct {
    int uart_num;                     /*!< UART port number */
    uart_config_t uart_config;       /*!< UART configuration */
    QueueHandle_t event_queue;        /*!< Queue for UART events */
    EventGroupHandle_t event_group;   /*!< Event group for signaling events */
    EventBits_t event_rx;             /*!< Event bit for RX events */    
} srv_ra4m1_uart_data_t;

static const char *TAG = "ra4m1_uart";

static srv_ra4m1_uart_data_t _self = {
    .uart_config = {
        .baud_rate = RA4M1_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    },
};

static void _uart_event_task(void *pvParameters) {
    uart_event_t event;
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(_self.event_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            switch (event.type) {
            //Rx event -> signal data availability to the main application
            case UART_DATA:
                // Ignore Rx data while programming
                if (ra4m1_ctrl_is_programming() == false) {
                    xEventGroupSetBits(_self.event_group, _self.event_rx);
                }
                break;
            //Event of UART buffer overflow
            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "UART HW fifo overflow");
                uart_flush_input(_self.uart_num);
                xQueueReset(_self.event_queue);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "UART ring buffer full");
                uart_flush_input(_self.uart_num);
                xQueueReset(_self.event_queue);
                break;                
            //Others
            default:
                ESP_LOGW(TAG, "Unexpected uart event (type: %d)", event.type);
                break;
            }
        }
    }
}

void ra4m1_uart_init(int uart_num, int baud_rate, int tx_pin, int rx_pin, EventGroupHandle_t event_group, EventBits_t event_rx) {
    _self.uart_num = uart_num;
    _self.uart_config.baud_rate = baud_rate;
    _self.event_group = event_group;
    _self.event_rx = event_rx;

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(_self.uart_num, &_self.uart_config));    
    ESP_ERROR_CHECK(uart_set_pin(_self.uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // Setup UART buffered IO with event queue
    ESP_ERROR_CHECK(uart_driver_install(_self.uart_num, RA4M1_UART_BUFFER_SIZE, RA4M1_UART_BUFFER_SIZE, RA4M1_UART_EVENT_QUEUE_SIZE, &_self.event_queue, 0));
    ESP_ERROR_CHECK(uart_flush(_self.uart_num));

    // Create a task to handle UART event from ISR (Stack overlow with size sets to 2048)
    xTaskCreate(_uart_event_task, "uart_event_task", 3072, NULL, 12, NULL);
}

void ra4m1_uart_rx(uart_msg_t *message) {
    size_t buffer_length;

    *message = (uart_msg_t){.payload=NULL, .len=0};

    // Ignore rx request while programming 
    if (ra4m1_ctrl_is_programming() == false) {
        if (uart_get_buffered_data_len(_self.uart_num, &buffer_length) == ESP_OK) {
            if (buffer_length > 0) {
                message->payload = malloc(buffer_length * sizeof(uint8_t));
                if (message->payload != NULL) {
                    int n_bytes = uart_read_bytes(_self.uart_num, message->payload, (uint32_t) buffer_length, 0);
                    if (n_bytes >= 0) {
                        message->len = (size_t) n_bytes;
                    } else {
                        ESP_LOGW(TAG, "Unable to read from uart");
                        free(message->payload);
                        message->payload = NULL;                    
                    }
                } else {
                    ESP_LOGW(TAG, "Unable to allocate buffer memory");
                }
            }
        } else {
            ESP_LOGW(TAG, "Unable to fetch uart data size");
        }
    }
}

int ra4m1_uart_tx(uart_msg_t *message) {
    int n_bytes = 0;

    // Ignore tx request while programming 
    if (ra4m1_ctrl_is_programming() == false) {
        n_bytes = uart_write_bytes(_self.uart_num, message->payload, message->len);
        if (n_bytes < 0) {
            ESP_LOGW(TAG, "Unable to write to uart");
        }
    }

    return n_bytes;
}