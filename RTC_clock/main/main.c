#include <stdio.h>
#include <string.h>
#include "ds3231.h"

#define SET_TIME_FROM_COMPILE 1 // Set to 1 to set the time, 0 to read the time
/**
 * @brief Get the compile time as a ds3231_time_t structure.
 *
 * This function retrieves the compile time and converts it to a ds3231_time_t structure.
 *
 * @return ds3231_time_t The current compile time.
 */
// Convert compiler time to ds3231_time_t
ds3231_time_t get_compile_time(void)
{
    ds3231_time_t t;

    // Parse time string: "15:47:10"
    int hour, min, sec;
    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);
    t.hour = hour;
    t.minute = min;
    t.second = sec;

    // Parse date string: "Jul 20 2025"
    int day, year;
    char month_str[4];
    sscanf(__DATE__, "%3s %d %d", month_str, &day, &year);

    t.date = day;
    t.year = year;

    // Convert month string to number
    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char *p = strstr(months, month_str);
    t.month = (p) ? ((p - months) / 3 + 1) : 1;

    // Day of week is unknown, just set to 1 (Sunday)
    t.day_of_week = 1;

    return t;
}
static inline void to_12h(uint8_t h24, uint8_t *h12, const char **ampm)
{
    *ampm = (h24 >= 12) ? "PM" : "AM";
    uint8_t h = h24 % 12;
    *h12 = (h == 0) ? 12 : h;
}

void app_main(void)
{
    // init I2C + (optional) scan
    i2c_bus_init();
    i2c_bus_scan();

#if SET_TIME_FROM_COMPILE
    ds3231_time_t t = get_compile_time();
    if (ds3231_set_time(&t) == ESP_OK)
    {
        printf("RTC set from compile time.\n");
    }
    else
    {
        printf("RTC set failed.\n");
    }
#endif

    ds3231_time_t now;
    while (1)
    {
        if (ds3231_get_time(&now) == ESP_OK)
        {
            uint8_t h12;
            const char *ampm;
            to_12h(now.hour, &h12, &ampm);
            printf("Now: %02u:%02u:%02u %s  %02u-%02u-%04u\n",
                   h12, now.minute, now.second, ampm,
                   now.month, now.date, now.year);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}