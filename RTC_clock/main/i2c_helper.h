#ifndef I2C_HELPER_H
#define I2C_HELPER_H

#include "esp_err.h"

esp_err_t i2c_master_init(void);
void i2c_scan(void);

#endif
