#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max7219.h"

void app_main(void)
{
    max7219_bus_cfg_t bus = {
        .spi_host = SPI2_HOST,
        .pin_mosi = 23,
        .pin_sclk = 18,
        .pin_cs = 5,
        .clock_hz = 1 * 1000 * 1000,
        .chain_len = 1};

    // 4 digits, low intensity, decode ON (7-segment numbers)
    max7219_t *h = max7219_init(&bus, /*active_digits*/ 4, /*intensity*/ 2, /*decode_bcd*/ true);

    // Show 12.34 (DP on digit2 from right)
    max7219_set_number(h, 0, 1230, 0b0000, /*blank_zero=*/true);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Example: HH:MM on 4 digits, DP on colon (pos 2)
    uint8_t hours = 7; // 07:35
    uint8_t mins = 35;

    max7219_set_digit(h, 0, 3, (hours / 10), false, /*blank_zero=*/true); // tens of hours -> blank if 0
    max7219_set_digit(h, 0, 2, (hours % 10), true, /*blank_zero=*/false); // ones of hours, DP=colon left dot
    max7219_set_digit(h, 0, 1, (mins / 10), false, /*blank_zero=*/false); // tens of minutes
    max7219_set_digit(h, 0, 0, (mins % 10), false, /*blank_zero=*/false); // ones of minutes

    vTaskDelay(pdMS_TO_TICKS(5000));

    // Simple counter 0000..9999
    for (uint32_t n = 0;; ++n)
    {
        max7219_set_number(h, 0, n, 0, true);
        vTaskDelay(pdMS_TO_TICKS(100));
        if (n > 2000)
            n = 0;
    }
}
