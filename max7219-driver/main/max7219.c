#include "max7219.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

#define REG_NOOP        0x00
#define REG_DIGIT0      0x01
#define REG_DIGIT1      0x02
#define REG_DIGIT2      0x03
#define REG_DIGIT3      0x04
#define REG_DIGIT4      0x05
#define REG_DIGIT5      0x06
#define REG_DIGIT6      0x07
#define REG_DIGIT7      0x08
#define REG_DECODE_MODE 0x09
#define REG_INTENSITY   0x0A
#define REG_SCAN_LIMIT  0x0B
#define REG_SHUTDOWN    0x0C
#define REG_DISPLAYTEST 0x0F

#define MAX_CHAIN 8          // max supported devices in the chain
#define DP_BIT    0x80       // decimal point bit (bit7)
#define CODEB_BLANK 0x0F     // Code-B blank symbol when decode is ON

// Optional: pass this as 'val' to force blank in set_digit
#ifndef MAX7219_BLANK
#define MAX7219_BLANK 0xFF
#endif

struct max7219_handle {
    spi_device_handle_t dev;
    uint8_t chain_len;
    uint8_t active_digits; // 1..8
    uint8_t decode_mask;   // bit per digit (1 = decode ON)
};

static const char* TAG = "MAX7219";

/* ====================== SPI helpers ====================== */

static esp_err_t tx_all(max7219_t* h, uint8_t reg, uint8_t data) {
    const int n = h->chain_len;
    if (n == 0 || n > MAX_CHAIN) return ESP_ERR_INVALID_SIZE;
    uint8_t tx[2 * MAX_CHAIN];   // two bytes per device
    for (int i = 0; i < n; ++i) { tx[2*i] = reg; tx[2*i+1] = data; }
    spi_transaction_t t = { .length = 16 * n, .tx_buffer = tx };
    return spi_device_transmit(h->dev, &t);
}

static esp_err_t tx_one(max7219_t* h, uint8_t dev_idx, uint8_t reg, uint8_t data) {
    const int n = h->chain_len;
    if (dev_idx >= n) return ESP_ERR_INVALID_ARG;
    if (n == 0 || n > MAX_CHAIN) return ESP_ERR_INVALID_SIZE;
    uint8_t tx[2 * MAX_CHAIN];
    for (int i = 0; i < n; ++i) {
        tx[2*i]   = (i == dev_idx) ? reg  : REG_NOOP;
        tx[2*i+1] = (i == dev_idx) ? data : 0x00;
    }
    spi_transaction_t t = { .length = 16 * n, .tx_buffer = tx };
    return spi_device_transmit(h->dev, &t);
}

/* ====================== Init & config ====================== */

max7219_t* max7219_init(const max7219_bus_cfg_t* bus,
                        uint8_t active_digits, uint8_t intensity, bool decode_bcd)
{
    if (!bus || bus->chain_len == 0 || bus->chain_len > MAX_CHAIN) return NULL;
    if (active_digits < 1 || active_digits > 8) return NULL;

    spi_bus_config_t bcfg = {
        .mosi_io_num = bus->pin_mosi,
        .miso_io_num = -1,
        .sclk_io_num = bus->pin_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    if (spi_bus_initialize(bus->spi_host, &bcfg, SPI_DMA_DISABLED) != ESP_OK) return NULL;

    spi_device_interface_config_t dcfg = {
        .clock_speed_hz = bus->clock_hz,
        .mode = 0,
        .spics_io_num = bus->pin_cs,
        .queue_size = 1,
    };

    max7219_t* h = (max7219_t*)calloc(1, sizeof(*h));
    if (!h) { spi_bus_free(bus->spi_host); return NULL; }

    if (spi_bus_add_device(bus->spi_host, &dcfg, &h->dev) != ESP_OK) {
        spi_bus_free(bus->spi_host);
        free(h);
        return NULL;
    }

    h->chain_len     = bus->chain_len;
    h->active_digits = active_digits;

    // Bring-up: configure while in shutdown (no flicker), then enable
    (void)tx_all(h, REG_SHUTDOWN, 0x00);
    uint8_t dmask = decode_bcd ? (uint8_t)((1u << active_digits) - 1u) : 0x00;
    (void)tx_all(h, REG_DECODE_MODE, dmask);
    h->decode_mask = dmask; // cache current decode per digit

    (void)tx_all(h, REG_SCAN_LIMIT,  (uint8_t)(active_digits - 1));
    (void)tx_all(h, REG_INTENSITY,   (uint8_t)(intensity & 0x0F));
    (void)tx_all(h, REG_DISPLAYTEST, 0x00);
    (void)tx_all(h, REG_SHUTDOWN,    0x01);

    // Clear only the visible digits with the correct blank per mode
    for (uint8_t d = 0; d < active_digits; ++d) {
        bool decode_on = ((h->decode_mask >> d) & 1u) != 0;
        uint8_t blank  = decode_on ? CODEB_BLANK : 0x00;
        (void)tx_all(h, (uint8_t)(REG_DIGIT0 + d), blank);
    }

    return h;
}

esp_err_t max7219_set_intensity(max7219_t* h, uint8_t intensity) {
    return tx_all(h, REG_INTENSITY, intensity & 0x0F);
}

esp_err_t max7219_set_decode(max7219_t* h, uint8_t decode_mask) {
    esp_err_t e = tx_all(h, REG_DECODE_MODE, decode_mask);
    if (e == ESP_OK) h->decode_mask = decode_mask; // keep cache in sync
    return e;
}

esp_err_t max7219_set_scan_limit(max7219_t* h, uint8_t last_digit) {
    if (last_digit > 7) return ESP_ERR_INVALID_ARG;
    return tx_all(h, REG_SCAN_LIMIT, last_digit);
}

esp_err_t max7219_set_shutdown(max7219_t* h, bool on) {
    return tx_all(h, REG_SHUTDOWN, on ? 0x01 : 0x00);
}

esp_err_t max7219_set_test(max7219_t* h, bool on) {
    return tx_all(h, REG_DISPLAYTEST, on ? 0x01 : 0x00);
}

esp_err_t max7219_clear(max7219_t* h) {
    // Clear only the configured active digits; use correct blank per digit mode
    for (uint8_t d = 0; d < h->active_digits; ++d) {
        bool decode_on = ((h->decode_mask >> d) & 1u) != 0;
        uint8_t blank  = decode_on ? CODEB_BLANK : 0x00;
        esp_err_t e = tx_all(h, (uint8_t)(REG_DIGIT0 + d), blank);
        if (e != ESP_OK) return e;
    }
    return ESP_OK;
}

/* ====================== Data writes ====================== */

esp_err_t max7219_write_raw(max7219_t* h, uint8_t dev, uint8_t digit_idx, uint8_t value) {
    if (digit_idx > 7) return ESP_ERR_INVALID_ARG;
    return tx_one(h, dev, (uint8_t)(REG_DIGIT0 + digit_idx), value);
}

esp_err_t max7219_set_digit(max7219_t* h, uint8_t dev, uint8_t pos,
                            uint8_t val, bool dp, bool blank_zero)
{
    if (pos > 7) return ESP_ERR_INVALID_ARG;

    // Per-position mode: decode ON uses Code-B symbols; raw uses segment bits
    bool decode_on  = ((h->decode_mask >> pos) & 1u) != 0;
    uint8_t blank   = decode_on ? CODEB_BLANK : 0x00;

    // Decide if this position should blank
    bool force_blank = (val == MAX7219_BLANK);
    bool do_blank    = force_blank || (blank_zero && val == 0);

    uint8_t out = do_blank
                ? (uint8_t)(blank | (dp ? DP_BIT : 0x00))
                : (uint8_t)((val & 0x0F) | (dp ? DP_BIT : 0x00));

    return tx_one(h, dev, (uint8_t)(REG_DIGIT0 + pos), out);
}

// Leading-zero suppression controlled by caller via 'blank_zero'
esp_err_t max7219_set_number(max7219_t* h, uint8_t dev,
                             uint32_t value, uint8_t dp_mask, bool blank_zero)
{
    const uint8_t digits = h->active_digits;
    uint8_t buf[8] = {0};

    // Extract digits LSB → MSB into buf[0..digits-1]
    uint32_t tmp = value;
    for (uint8_t i = 0; i < digits; ++i) {
        buf[i] = (uint8_t)(tmp % 10u);
        tmp   /= 10u;
    }

    // Find most significant non-zero index (MSNZ); if value==0, MSNZ = 0
    int8_t msnz = (int8_t)digits - 1;
    while (msnz > 0 && buf[msnz] == 0) msnz--;

    for (uint8_t pos = 0; pos < digits; ++pos) {
        bool dp = ((dp_mask >> pos) & 1u) != 0;

        // Only blank positions strictly above MSNZ when blank_zero==true
        bool leading_area = (pos > (uint8_t)msnz);
        bool should_blank = (blank_zero && leading_area);

        esp_err_t e = max7219_set_digit(
            h, dev, pos,
            should_blank ? 0 : buf[pos],
            dp,
            should_blank   // pass blank_zero=true only for leading area
        );
        if (e != ESP_OK) return e;
    }
    return ESP_OK;
}

/* ====================== Matrix helpers ====================== */

esp_err_t max7219_set_rows(max7219_t* h, uint8_t dev, const uint8_t rows[8]) {
    for (uint8_t r = 0; r < 8; ++r) {
        esp_err_t e = max7219_write_raw(h, dev, r, rows[r]);
        if (e != ESP_OK) return e;
    }
    return ESP_OK;
}

/* ====================== Introspection ====================== */

uint8_t max7219_active_digits(const max7219_t* h) { return h->active_digits; }
uint8_t max7219_chain_len(const max7219_t* h)     { return h->chain_len; }
