#include <stdio.h>
#include "i2c_helper.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

#define I2C_MASTER_SCL_IO GPIO_NUM_18 /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21 /*!< GPIO number used for I2C master data */
#define I2C_MASTER_NUM I2C_NUM_0      /*!< I2C port number for master */
#define I2C_MASTER_FREQ_HZ 100000     /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS 1000    /*!< I2C master timeout in milliseconds */

static const char *TAG = "I2C_HELPER";

esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C initialized on SDA:%d SCL:%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

void i2c_scan(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");

    for (uint8_t address = 0; address < 127; address++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Found device at address 0x%02X", address);
        }
    }

    ESP_LOGI(TAG, "I2C scan complete.");
}