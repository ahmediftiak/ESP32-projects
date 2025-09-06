#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// --------- Defaults (override via -D or before including) ----------
#ifndef I2C_MASTER_SCL_IO
#define I2C_MASTER_SCL_IO      22
#endif
#ifndef I2C_MASTER_SDA_IO
#define I2C_MASTER_SDA_IO      21
#endif
#ifndef I2C_MASTER_NUM
#define I2C_MASTER_NUM         I2C_NUM_0
#endif
#ifndef I2C_MASTER_FREQ_HZ
#define I2C_MASTER_FREQ_HZ     100000
#endif
#ifndef I2C_MASTER_TIMEOUT_MS
#define I2C_MASTER_TIMEOUT_MS  1000
#endif
#ifndef DS3231_I2C_ADDRESS
#define DS3231_I2C_ADDRESS     0x68
#endif
#ifndef DS3231_REG_TIME
#define DS3231_REG_TIME        0x00
#endif

/**
 * @brief High-level time container for DS3231.
 *
 * - hour is returned in 24-hour format regardless of RTC mode.
 * - day_of_week: 1–7, with 1=Sunday (datasheet convention).
 */
typedef struct {
    uint8_t  second;       ///< 0–59
    uint8_t  minute;       ///< 0–59
    uint8_t  hour;         ///< 0–23 (library handles 12h→24h on read)
    uint8_t  day_of_week;  ///< 1–7 (1=Sunday per DS3231)
    uint8_t  date;         ///< 1–31
    uint8_t  month;        ///< 1–12
    uint16_t year;         ///< e.g. 2025
} ds3231_time_t;

/**
 * @brief Initialize I²C master on the configured port/pins/frequency.
 *
 * @return ESP_OK on success; error code otherwise.
 */
esp_err_t i2c_bus_init(void);

/**
 * @brief Scan I²C bus (0x01–0x7E) and log discovered addresses.
 */
void i2c_bus_scan(void);

/**
 * @brief Read 7 raw BCD bytes from DS3231 time registers (0x00..0x06).
 *
 * @param[out] buf7 Pointer to a 7-byte buffer.
 * @return ESP_OK on success; error code otherwise.
 */
esp_err_t ds3231_read_raw(uint8_t *buf7);

/**
 * @brief Read current time and convert to decimal (24-hour).
 *
 * Applies CH masking, 12/24-hour decoding, and century bit handling.
 *
 * @param[out] t Filled with current time.
 * @return ESP_OK on success; error code otherwise.
 */
esp_err_t ds3231_get_time(ds3231_time_t *t);

/**
 * @brief Write current time to DS3231 (encodes as 24-hour).
 *
 * Encodes decimal → BCD and writes registers 0x00..0x06.
 *
 * @param[in] t Time to set (year 2000–2199 supported via century bit).
 * @return ESP_OK on success; error code otherwise.
 */
esp_err_t ds3231_set_time(const ds3231_time_t *t);

#ifdef __cplusplus
}
#endif
#endif // DS3231_H
