#include "ds3231.h"

#include "freertos/FreeRTOS.h"        // pdMS_TO_TICKS
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

#define I2C_PORT              I2C_MASTER_NUM
static const char *TAG_I2C   = "I2C_HELPER";
static const char *TAG_RTC   = "DS3231";

// ---------------- BCD helpers ----------------
static inline uint8_t bcd_to_decimal(uint8_t bcd) {
    return (uint8_t)((bcd >> 4) * 10U + (bcd & 0x0FU));
}
static inline uint8_t decimal_to_bcd(uint8_t dec) {
    return (uint8_t)(((dec / 10U) << 4) | (dec % 10U));
}

// ================= I²C helper =================
esp_err_t i2c_bus_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        // .clk_flags = 0,
    };

    esp_err_t ret = i2c_param_config(I2C_PORT, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_I2C, "i2c_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG_I2C, "I2C driver already installed on port %d", I2C_PORT);
        return ESP_OK;
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG_I2C, "i2c_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG_I2C, "I2C init OK on SDA:%d SCL:%d @%u Hz",
             (int)I2C_MASTER_SDA_IO, (int)I2C_MASTER_SCL_IO, (unsigned)I2C_MASTER_FREQ_HZ);
    return ESP_OK;
}

void i2c_bus_scan(void)
{
    ESP_LOGI(TAG_I2C, "Scanning I2C bus on port %d...", I2C_PORT);
    for (uint8_t address = 1; address < 0x7F; address++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG_I2C, "Found device at 0x%02X", address);
        }
    }
    ESP_LOGI(TAG_I2C, "I2C scan complete.");
}

// ================= DS3231 driver =================
esp_err_t ds3231_read_raw(uint8_t *buf7)
{
    if (!buf7) {
        ESP_LOGE(TAG_RTC, "buf7 is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Set register pointer to 0x00
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, DS3231_REG_TIME, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_RTC, "Failed to set pointer: %s", esp_err_to_name(ret));
        return ret;
    }

    // Read 7 bytes (0x00..0x06)
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buf7, 6, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, buf7 + 6, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG_RTC, "Failed to read: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG_RTC, "Read 7 bytes from DS3231");
    return ESP_OK;
}

esp_err_t ds3231_set_time(const ds3231_time_t *t)
{
    if (!t) {
        ESP_LOGE(TAG_RTC, "time ptr is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[7];
    data[0] = decimal_to_bcd(t->second & 0x7F);
    data[1] = decimal_to_bcd(t->minute & 0x7F);
    data[2] = decimal_to_bcd(t->hour   & 0x3F);            // write as 24h
    data[3] = decimal_to_bcd(t->day_of_week & 0x07);
    data[4] = decimal_to_bcd(t->date & 0x3F);
    // month + century
    uint8_t month_bcd = decimal_to_bcd(t->month & 0x1F);
    uint16_t base = (t->year >= 2100) ? 2100U : 2000U;
    if (base == 2100U) month_bcd |= 0x80;                  // set century bit
    data[5] = month_bcd;
    data[6] = decimal_to_bcd((uint8_t)(t->year - base));   // 0..99

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, DS3231_REG_TIME, true);
    i2c_master_write(cmd, data, sizeof(data), true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_RTC, "Failed to set time: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG_RTC, "Time set OK");
    return ESP_OK;
}

esp_err_t ds3231_get_time(ds3231_time_t *t)
{
    if (!t) {
        ESP_LOGE(TAG_RTC, "time ptr is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[7];
    esp_err_t ret = ds3231_read_raw(raw);
    if (ret != ESP_OK) return ret;

    t->second = bcd_to_decimal(raw[0] & 0x7F);
    t->minute = bcd_to_decimal(raw[1] & 0x7F);

    // Hour: handle 12/24 mode
    uint8_t hr = raw[2];
    if (hr & 0x40) {
        // 12h mode: bit5=AM/PM, bits4:0=hour BCD
        uint8_t hour12 = bcd_to_decimal(hr & 0x1F);
        uint8_t pm     = (hr & 0x20) ? 1 : 0;
        t->hour = (uint8_t)((hour12 % 12) + (pm ? 12 : 0));
    } else {
        t->hour = bcd_to_decimal(hr & 0x3F);
    }

    t->day_of_week = bcd_to_decimal(raw[3] & 0x07);
    t->date        = bcd_to_decimal(raw[4] & 0x3F);

    uint8_t month_reg = raw[5];
    t->month = bcd_to_decimal(month_reg & 0x1F);
    uint16_t century_base = (month_reg & 0x80) ? 2100U : 2000U;
    t->year = (uint16_t)bcd_to_decimal(raw[6]) + century_base;

    ESP_LOGI(TAG_RTC, "Time read & converted");
    return ESP_OK;
}
