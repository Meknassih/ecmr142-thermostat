#ifndef BMP280_H
#define BMP280_H

#include <stdbool.h>
#include "hardware/i2c.h"

bool bmp280_init(i2c_inst_t *i2c);
bool bmp280_read(float *temp_c, float *pressure_pa);

#endif
