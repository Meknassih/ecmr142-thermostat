#ifndef AHT20_H
#define AHT20_H

#include <stdbool.h>
#include "hardware/i2c.h"

bool aht20_init(i2c_inst_t *i2c);
bool aht20_read(float *temp_c, float *humidity);

#endif
