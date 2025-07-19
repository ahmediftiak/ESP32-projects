#include <stdio.h>
#include <string.h>
#include "i2c_helper.h"
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
ds3231_time_t get_compile_time(void) {
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

void app_main(void)
{
    // Initialize I2C
    i2c_master_init();

#if SET_TIME_FROM_COMPILE
    // set the time on DS3231
    ds3231_time_t t = get_compile_time(); // Get the compile time

    if (ds3231_set_time(&t) == ESP_OK)
    {
        printf("Time set successfully on DS3231.\n");
    }
    else
    {
        printf("Failed to set time on DS3231.\n");
    }
#endif
    // Get the current time from DS3231
    ds3231_time_t now;

    if (ds3231_get_time(&now) == ESP_OK)
    {
        printf("Now: %02d:%02d:%02d %02d-%02d-%04d\n",
               now.hour, now.minute, now.second,
               now.date, now.month, now.year);
    }
    else
    {
        printf("Failed to read time from DS3231.\n");
    }
}
    