#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Define pin 
#define LED_GPIO    GPIO_NUM_8
#define BUTTON_GPIO GPIO_NUM_10

// Initialize LED and button GPIOs
void init_gpio(void){

    // LED as output
    gpio_config_t led_conf ={
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);


    gpio_config_t button_conf ={
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_conf);
}

void app_main(void){

    init_gpio();

    int led_state = 0;
    int last_button_state = 1;

    while (1) {
        int button_state = gpio_get_level(BUTTON_GPIO);

        // Detect falling edge (1 -> 0)
        if (last_button_state == 1 && button_state == 0){
            led_state = !led_state;
            gpio_set_level(LED_GPIO, led_state);
            printf("Button: %d, LED: %d\n", button_state, led_state);
            vTaskDelay(pdMS_TO_TICKS(200)); // debounce delay
            }

        last_button_state = button_state;
        vTaskDelay(pdMS_TO_TICKS(10)); // Pulling interval
    }
}
