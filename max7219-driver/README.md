# MAX7219 Driver for ESP32 (ESP-IDF)

Reusable driver for the **MAX7219 LED controller chip**. Supports both **7-segment** and **8×8 dot-matrix** displays.

---

## ✨ Features
- Modular driver (`max7219.c` / `max7219.h`)
- Code-B decode mode (easy display of numbers 0–9, A–F)
- Raw (no-decode) mode for custom segment/matrix control
- Functions for:
  - Writing raw values to digits/rows
  - Displaying single digits with optional decimal point
  - Displaying integers across multiple digits
  - Updating full rows (for dot-matrix)
- Leading-zero suppression option (useful for digital clocks)

---

## 🔌 Wiring (example: 4-digit 7-segment)

| ESP32 GPIO | MAX7219 Pin |
|:----------:|:-----------:|
| GPIO23     | DIN         |
| GPIO18     | CLK         |
| GPIO5      | CS / LOAD   |

*Note: VCC = 5V, GND = GND.*

---

## 🚀 Usage

Build and flash the example:

```bash
cd max7219-driver
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 📖 Example Code

```c
#include "max7219.h"

void app_main(void) {
    max7219_bus_cfg_t bus = {
        .spi_host  = SPI2_HOST,
        .pin_mosi  = 23,
        .pin_sclk  = 18,
        .pin_cs    = 5,
        .clock_hz  = 1 * 1000 * 1000,
        .chain_len = 1,
    };

    // Initialize with 4 active digits, medium brightness, decode mode ON
    max7219_t* h = max7219_init(&bus, 4, 4, true);

    // Show number 1234, with DP on the second digit
    max7219_set_number(h, 0, 1234, 0b0010, false);
}
```

---

## 🛠 Requirements
- ESP-IDF v5.x
- ESP32 development board
- MAX7219-based 7-segment or dot-matrix module

---

## 📜 License
MIT License  
(c) 2025 Iftiak Ahmed