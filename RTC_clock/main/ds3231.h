/**
 * @file ds3231.h
 * @brief Header file for DS3231 RTC driver.
 *
 * This file contains the definitions and function prototypes for interacting
 * with the DS3231 Real-Time Clock (RTC) using I2C communication.
 */

#ifndef DS3231_H
#define DS3231_H

#include "esp_err.h"
#include <stdint.h>

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day_of_week; // 1–7 (Sun–Sat)
    uint8_t date;        // Day of month
    uint8_t month;
    uint16_t year;
} ds3231_time_t;


/**
 * @brief Read raw time data from the DS3231 RTC.
 *
 * This function reads 7 bytes of raw time data from the DS3231 RTC starting
 * from register 0x00. The data is stored in the provided buffer.
 *
 * @param data Pointer to a buffer where the raw time data will be stored.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t ds3231_read_raw(uint8_t *data);

/**
 * @brief Get the current time from the DS3231 RTC.
 *
 * This function reads the current time from the DS3231 RTC and converts it
 * from BCD format to a human-readable format stored in a ds3231_time_t structure.
 *
 * @param time Pointer to a ds3231_time_t structure where the current time will be stored.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t ds3231_get_time(ds3231_time_t *time);

/**
 * @brief Set the current time on the DS3231 RTC.
 *
 * This function sets the current time on the DS3231 RTC using a ds3231_time_t structure.
 * The time is converted from decimal to BCD format before being written to the RTC.
 *
 * @param time Pointer to a ds3231_time_t structure containing the time to set.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t ds3231_set_time(const ds3231_time_t *time);

#endif // DS3231_H

