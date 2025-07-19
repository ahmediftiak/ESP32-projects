# ESP32 Projects Collection

This repository contains a collection of ESP32 projects developed using the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html) framework. The focus is on low-level embedded development, reusable drivers, and hands-on peripheral control.

---

## 📁 Projects

### ⏰ `RTC_clock/`
A project demonstrating communication with a DS3231 Real-Time Clock (RTC) over I2C using a custom driver.

**Features:**
- Custom I2C helper driver
- DS3231 read/write time
- Set RTC from compile time or system local time
- I2C bus scanning

**Wiring:**

| ESP32 GPIO | DS3231 Pin |
|:----------:|:----------:|
| GPIO21     | SDA        |
| GPIO18     | SCL        |

**Usage:**
```bash
cd RTC_clock
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

---

### 💡 `led_toggle/`
A basic project to toggle an LED with a push button using GPIO interrupts.

**Features:**
- GPIO input with interrupt on button press
- Debounced toggle logic
- Configurable LED pin

**Usage:**
```bash
cd led_toggle
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

---

## 🛠 Requirements

- ESP-IDF v5.x
- ESP32 development board (e.g., ESP32-S3)
- USB access via `/dev/ttyACM0` or equivalent serial port

---

## 📦 Structure

```
ESP32-projects/
├── RTC_clock/        # DS3231 RTC with I2C driver
├── led_toggle/       # LED + button GPIO toggle example
└── .gitignore        # Excludes build artifacts and temp files
```

---

## 📌 Notes

- Each project is standalone with its own `sdkconfig`.
- Shared drivers like `i2c_helper` and `ds3231` live within project folders.
- Reusable component structure planned for future versions.

---

## ✍️ Author

**Ahmed Iftiak**  
Learning embedded systems through practical ESP32 projects.

---

## 📜 License

This repository is open-sourced under the MIT License.
