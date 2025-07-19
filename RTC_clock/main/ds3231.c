#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "ds3231.h"

#define DS3231_I2C_ADDRESS 0x68 /*!< I2C address for DS3231 RTC */
#define I2C_PORT I2C_NUM_0      /*!< I2C port number for master */
#define DS3231_REG_TIME 0x00    /*!< Register address for time data */

static const char *TAG = "DS3231_RAW";

// Function to convert BCD to decimal
static uint8_t bcd_to_decimal(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}
 // Function to convert decimal to BCD
static uint8_t decimal_to_bcd(uint8_t decimal)
{
    return ((decimal / 10) << 4) | (decimal % 10);
}

/**
 * @brief Read raw time data from the DS3231 RTC.
 *
 * This function reads 7 bytes of raw time data from the DS3231 RTC starting
 * from register 0x00. The data is stored in the provided buffer.
 *
 * @param data Pointer to a buffer where the raw time data will be stored.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t ds3231_read_raw(uint8_t *data)
{
    // Check if the data pointer is valid
    if (data == NULL)
    {
        ESP_LOGE(TAG, "Data buffer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create(); // Create a new I2C command link
    // STEP 1: Write the starting register address (0x00)
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, DS3231_REG_TIME, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd); // Delete the command link after use
    // Check if the write operation was successful
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read time from DS3231: %s", esp_err_to_name(ret));
        return ret;
    }

    // Read 7 bytes of time data
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 6, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, data + 6, I2C_MASTER_NACK); // // Last byte gets NACK
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd); // Delete the command link after use

    // Check if the read operation was successful
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read time data from DS3231: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Time data read successfully from DS3231");
    return ESP_OK;
}
/**
 * @brief Set the current time on the DS3231 RTC.
 *
 * This function converts a ds3231_time_t structure to raw BCD format and writes
 * it to the DS3231 RTC.
 *
 * @param time Pointer to a ds3231_time_t structure containing the time to set.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t ds3231_set_time(const ds3231_time_t *time)
{
    // Check if the time pointer is valid
    if (time == NULL)
    {
        ESP_LOGE(TAG, "Time structure is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t data[7]; // Buffer to hold raw time data
    // Convert the time structure to raw BCD format
    data[0] = decimal_to_bcd(time->second); // Second
    data[1] = decimal_to_bcd(time->minute); // Minute
    data[2] = decimal_to_bcd(time->hour);   // Hour
    data[3] = decimal_to_bcd(time->day_of_week); // Day of week
    data[4] = decimal_to_bcd(time->date);   // Date
    data[5] = decimal_to_bcd(time->month);  // Month
    data[6] = decimal_to_bcd(time->year - 2000); // Year (offset by 2000)
    // Write the raw data to the DS3231 RTC
        i2c_cmd_handle_t cmd = i2c_cmd_link_create(); // Create a new I2C command link
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);
    i2c_master_write(cmd, data, sizeof(data), true); // Write the 7 bytes of time data
    i2c_master_stop(cmd);

    esp_err_t ret;
    // Execute the command
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd); // Delete the command link after use

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set time on DS3231: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Time set successfully on DS3231");
    return ESP_OK;
}

/**
 * @brief Get the current time from the DS3231 RTC.
 *
 * This function reads the current time from the DS3231 RTC and converts it
 * from BCD format to a human-readable format stored in a ds3231_time_t structure.
 *
 * @param time Pointer to a ds3231_time_t structure where the current time will be stored.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t ds3231_get_time(ds3231_time_t *time)
{
    // Read raw time data from DS3231
    if (time == NULL)
    {
        ESP_LOGE(TAG, "Time buffer is NULL");
        return ESP_ERR_INVALID_ARG; // Return error if time buffer is NULL
    }
    uint8_t raw[7]; // Buffer to hold raw time data
    esp_err_t ret = ds3231_read_raw(raw);
    if (ret != ESP_OK)
    {
        return ret; // Return error if reading raw data failed
    }
    // Convert BCD to decimal for each byte
    time->second = bcd_to_decimal(raw[0] & 0x7F);      // Mask to ignore the CH bit
    time->minute = bcd_to_decimal(raw[1] & 0x7F);      // Mask to ignore the 12/24 hour bit
    time->hour = bcd_to_decimal(raw[2] & 0x3F);        // Mask to ignore the 12/24 hour bit
    time->day_of_week = bcd_to_decimal(raw[3] & 0x07); // Mask to ignore the unused bits
    time->date = bcd_to_decimal(raw[4] & 0x3F);        // Mask to ignore the unused bits
    time->month = bcd_to_decimal(raw[5] & 0x1F);       // Mask to ignore the unused bits
    time->year = bcd_to_decimal(raw[6]) + 2000;        //

    ESP_LOGI(TAG, "Time data converted from BCD to decimal successfully");
    return ESP_OK;
}
