#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_PIN1 7

#define OUTPUT_PIN 11
#define INPUT_PIN 9

static void signal_handler(void *arg);
static void timer_callback(void* arg);
const char *TAG = "Signal Activation Delay: ";
static esp_timer_handle_t my_timer;

static portMUX_TYPE mux    = portMUX_INITIALIZER_UNLOCKED;
volatile uint64_t stop_time = 0;
volatile uint64_t start_time = 0;

void setup(void)
{
    gpio_config_t led_conf = {
        .pin_bit_mask = 1ULL << LED_PIN1,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&led_conf);
    gpio_set_level((gpio_num_t)LED_PIN1, true); 

    gpio_config_t out_conf = {
        .pin_bit_mask = 1ULL << OUTPUT_PIN,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&out_conf);
    gpio_set_level((gpio_num_t)OUTPUT_PIN, false); 

    gpio_config_t in_config = {
        .pin_bit_mask = 1ULL << INPUT_PIN,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,//GPIO_INTR_POSEDGE
    };
    gpio_config(&in_config);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add((gpio_num_t)INPUT_PIN, signal_handler, NULL);

    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "my_timer"
    };
    esp_timer_create(&timer_args, &my_timer);

    ESP_LOGI(TAG, "Setup complete.");
}

static void timer_callback(void* arg) {    
    stop_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Activation time: %llu us", (stop_time - start_time) );
}

void IRAM_ATTR signal_handler(void *arg) {     
    portENTER_CRITICAL_ISR(&mux);        
        esp_timer_stop(my_timer);
    portEXIT_CRITICAL_ISR(&mux); 
}


void app_main() {
    setup();
    
    while(1) {
        gpio_set_level((gpio_num_t)INPUT_PIN, true); 
        start_time = esp_timer_get_time(); 
        portENTER_CRITICAL_ISR(&mux);            
            esp_timer_start_once(my_timer, 1000000);
        portEXIT_CRITICAL_ISR(&mux); 
        
        // vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level((gpio_num_t)INPUT_PIN, false); 
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
