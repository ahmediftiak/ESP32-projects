/**
 * @file max7219.h
 * @brief Driver for the MAX7219 LED controller (7-segment and 8×8 matrix).
 *
 * This driver supports both Code-B decode mode (for 7-segment digits)
 * and raw (no-decode) mode (for dot-matrix or custom segment patterns).
 *
 * Typical usage:
 * @code
 * max7219_bus_cfg_t bus = {
 *     .spi_host  = SPI2_HOST,
 *     .pin_mosi  = 23,
 *     .pin_sclk  = 18,
 *     .pin_cs    = 5,
 *     .clock_hz  = 1*1000*1000,
 *     .chain_len = 1,
 * };
 *
 * // 4 digits active, low brightness, decode ON for digits
 * max7219_t* h = max7219_init(&bus, 4, 2, true);
 *
 * // Show 12.34 (DP on pos2 from the right)
 * max7219_set_number(h, 0, 1234, 0b0100, blank_zero);
 * @endcode
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque driver handle */
typedef struct max7219_handle max7219_t;

/** @brief Sentinel value for @p val in max7219_set_digit() to force blanking. */
#ifndef MAX7219_BLANK
#define MAX7219_BLANK 0xFF
#endif

/** @brief SPI and chain configuration */
typedef struct {
    spi_host_device_t spi_host; /**< ESP32 SPI host, e.g. SPI2_HOST */
    int pin_mosi;               /**< GPIO for MOSI (DIN) */
    int pin_sclk;               /**< GPIO for SCLK */
    int pin_cs;                 /**< GPIO for CS/LOAD */
    int clock_hz;               /**< SPI clock speed in Hz (e.g., 1 MHz) */
    uint8_t chain_len;          /**< Number of MAX7219 devices daisy-chained (1..8) */
} max7219_bus_cfg_t;

/**
 * @brief Initialize a MAX7219 chain on the given SPI bus.
 *
 * The device is configured while in shutdown (no flicker), then enabled. The
 * initial decode mask is applied to the lowest @p active_digits positions.
 *
 * @param bus           SPI/chain configuration (non-NULL).
 * @param active_digits Number of digit/row indices to use per device (1..8).
 * @param intensity     Initial brightness (0x00..0x0F).
 * @param decode_bcd    True = enable Code-B decode for those digits, false = raw mode.
 * @return Driver handle on success, or NULL on failure.
 */
max7219_t* max7219_init(const max7219_bus_cfg_t* bus,
                        uint8_t active_digits, uint8_t intensity, bool decode_bcd);

/* -------------------------------------------------------------------------- */
/* Configuration API                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Set global brightness intensity for all devices.
 * @param h Driver handle
 * @param intensity 0x00..0x0F
 */
esp_err_t max7219_set_intensity(max7219_t* h, uint8_t intensity);

/**
 * @brief Enable Code-B decode per digit position (broadcast to all devices).
 *
 * Each bit in @p decode_mask corresponds to a digit index (bit0 = DIG0, …, bit7 = DIG7).
 *  - Bit=1  → Decode ON (Code-B symbol mapping)
 *  - Bit=0  → Decode OFF (raw segment bits)
 *
 * The driver caches this mask to choose the correct blanking code per position.
 *
 * @param h           Driver handle
 * @param decode_mask Bitmask of digits with decode enabled
 */
esp_err_t max7219_set_decode(max7219_t* h, uint8_t decode_mask);

/**
 * @brief Set how many digit indices are actively scanned (per device).
 *
 * @param h         Driver handle
 * @param last_digit Highest digit index (0..7). E.g., 3 → DIG0..DIG3 scanned.
 */
esp_err_t max7219_set_scan_limit(max7219_t* h, uint8_t last_digit);

/**
 * @brief Enter/exit shutdown (low-power).
 * @param h  Driver handle
 * @param on True = normal operation, False = shutdown
 */
esp_err_t max7219_set_shutdown(max7219_t* h, bool on);

/**
 * @brief Enter/exit display-test mode (all segments lit).
 * @param h  Driver handle
 * @param on True = test ON, False = normal operation
 */
esp_err_t max7219_set_test(max7219_t* h, bool on);

/**
 * @brief Clear all visible digits/rows on all devices.
 *
 * Uses the correct blank code per position based on the current decode mask.
 * Only the first @c active_digits positions are cleared.
 *
 * @param h Driver handle
 */
esp_err_t max7219_clear(max7219_t* h);

/* -------------------------------------------------------------------------- */
/* Display API                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Write a raw 8-bit value to a digit/row (REG_DIGITn).
 *
 * - In NO-DECODE (raw) mode: bits map to DP,A,B,C,D,E,F,G.
 * - In DECODE mode: only 0..0x0F are meaningful (Code-B symbols);
 *   0x0F is the blank symbol, DP is still bit7.
 *
 * @param h         Driver handle
 * @param dev       Device index in the chain (0..chain_len-1)
 * @param digit_idx Digit/row index (0..7)
 * @param value     Raw value to write
 */
esp_err_t max7219_write_raw(max7219_t* h, uint8_t dev, uint8_t digit_idx, uint8_t value);

/**
 * @brief Display a single digit (0..15) with optional decimal point.
 *
 * If @p blank_zero is true and @p val == 0, the digit is blanked instead of showing "0".
 * You can also force blank by passing @c MAX7219_BLANK as @p val (useful for clocks).
 *
 * Blanking respects the current mode of that position:
 *  - Decode ON  → 0x0F (Code-B blank) is written; DP bit is still honored.
 *  - Decode OFF → 0x00 (all segments off) is written; DP bit is still honored.
 *
 * @param h          Driver handle
 * @param dev        Device index in the chain
 * @param pos        Digit position (0..7, 0 = rightmost)
 * @param val        0..15 for digits (0..9,A..F), or @c MAX7219_BLANK to force blank
 * @param dp         True to enable decimal point
 * @param blank_zero True to blank when @p val == 0 (leading-zero suppression per digit)
 */
esp_err_t max7219_set_digit(max7219_t* h, uint8_t dev, uint8_t pos,
                            uint8_t val, bool dp, bool blank_zero);

/**
 * @brief Display an integer right-aligned across the active digits.
 *
 * If @p blank_zero is true, only the *leading* zeros are blanked (interior zeros remain).
 * When @p value == 0 and @p blank_zero == true, the rightmost digit shows "0" and the
 * remaining higher positions are blanked.
 *
 * @param h          Driver handle
 * @param dev        Device index in the chain
 * @param value      Number to display (truncated to @c active_digits)
 * @param dp_mask    Bitmask of decimal points (bit0 = pos0/rightmost)
 * @param blank_zero True to blank leading zeros; false to zero-pad
 */
esp_err_t max7219_set_number(max7219_t* h, uint8_t dev,
                             uint32_t value, uint8_t dp_mask, bool blank_zero);

/* -------------------------------------------------------------------------- */
/* Dot-matrix helpers (NO-DECODE mode)                                        */
/* -------------------------------------------------------------------------- */

/**
 * @brief Set a single 8-LED row (digit_idx) from a bitmask.
 * @param h    Driver handle
 * @param dev  Device index in chain
 * @param row  Row index (0..7)
 * @param bits Bitmask of 8 LEDs (MSB = column7, LSB = column0)
 */
static inline esp_err_t max7219_set_row(max7219_t* h, uint8_t dev, uint8_t row, uint8_t bits) {
    return max7219_write_raw(h, dev, row, bits);
}

/**
 * @brief Set all 8 rows of a device at once.
 * @param h    Driver handle
 * @param dev  Device index in chain
 * @param rows Array of 8 bytes (rows[0] = row0/top, rows[7] = row7/bottom)
 */
esp_err_t max7219_set_rows(max7219_t* h, uint8_t dev, const uint8_t rows[8]);

/* -------------------------------------------------------------------------- */
/* Introspection helpers                                                      */
/* -------------------------------------------------------------------------- */

/** @return Number of active digits configured at init (1..8). */
uint8_t max7219_active_digits(const max7219_t* h);
/** @return Chain length (number of devices daisy-chained). */
uint8_t max7219_chain_len(const max7219_t* h);

#ifdef __cplusplus
}
#endif
