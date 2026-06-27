#include "bmp280.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define BMP280_I2C_ADDR     0x77
#define BMP280_REG_ID       0xD0
#define BMP280_REG_CTRL     0xF4
#define BMP280_REG_CONFIG   0xF5
#define BMP280_REG_CALIB    0x88
#define BMP280_REG_DATA     0xF7
#define BMP280_CHIP_ID      0x58

static i2c_inst_t *i2c_port;
static bool initialized;

static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

static float t_fine;

static bool read_regs(uint8_t reg, uint8_t *buf, int len) {
    if (i2c_write_blocking(i2c_port, BMP280_I2C_ADDR, &reg, 1, true) < 0) return false;
    if (i2c_read_blocking(i2c_port, BMP280_I2C_ADDR, buf, len, false) < 0) return false;
    return true;
}

static bool write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_write_blocking(i2c_port, BMP280_I2C_ADDR, buf, 2, false) >= 0;
}

bool bmp280_init(i2c_inst_t *i2c) {
    i2c_port = i2c;

    uint8_t id = 0;
    if (!read_regs(BMP280_REG_ID, &id, 1)) return false;
    if (id != BMP280_CHIP_ID) return false;

    uint8_t c[24] = {0};
    if (!read_regs(BMP280_REG_CALIB, c, 24)) return false;

    dig_T1 = (uint16_t)((c[1] << 8) | c[0]);
    dig_T2 = (int16_t)((c[3] << 8) | c[2]);
    dig_T3 = (int16_t)((c[5] << 8) | c[4]);
    dig_P1 = (uint16_t)((c[7] << 8) | c[6]);
    dig_P2 = (int16_t)((c[9] << 8) | c[8]);
    dig_P3 = (int16_t)((c[11] << 8) | c[10]);
    dig_P4 = (int16_t)((c[13] << 8) | c[12]);
    dig_P5 = (int16_t)((c[15] << 8) | c[14]);
    dig_P6 = (int16_t)((c[17] << 8) | c[16]);
    dig_P7 = (int16_t)((c[19] << 8) | c[18]);
    dig_P8 = (int16_t)((c[21] << 8) | c[20]);
    dig_P9 = (int16_t)((c[23] << 8) | c[22]);

    if (!write_reg(BMP280_REG_CTRL, 0x27)) return false;
    if (!write_reg(BMP280_REG_CONFIG, 0xA0)) return false;

    initialized = true;
    return true;
}

bool bmp280_read(float *temp_c, float *pressure_pa) {
    if (!initialized) return false;

    uint8_t d[6] = {0};
    if (!read_regs(BMP280_REG_DATA, d, 6)) return false;

    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);

    float var1, var2;
    var1 = (((float)adc_T) / 16384.0f - ((float)dig_T1) / 1024.0f) * ((float)dig_T2);
    var2 = ((((float)adc_T) / 131072.0f - ((float)dig_T1) / 8192.0f) *
            (((float)adc_T) / 131072.0f - ((float)dig_T1) / 8192.0f)) * ((float)dig_T3);
    t_fine = var1 + var2;
    float T = (var1 + var2) / 5120.0f;

    var1 = ((float)t_fine / 2.0f) - 64000.0f;
    var2 = var1 * var1 * ((float)dig_P6) / 32768.0f;
    var2 = var2 + var1 * ((float)dig_P5) * 2.0f;
    var2 = (var2 / 4.0f) + (((float)dig_P4) * 65536.0f);
    var1 = (((float)dig_P3) * var1 * var1 / 524288.0f + ((float)dig_P2) * var1) / 524288.0f;
    var1 = (1.0f + var1 / 32768.0f) * ((float)dig_P1);
    float p = 1048576.0f - (float)adc_P;
    p = (p - (var2 / 4096.0f)) * 6250.0f / var1;
    var1 = ((float)dig_P7) * p * p / 2147483648.0f;
    var2 = p * ((float)dig_P6) / 32768.0f;
    p = p + (var1 + var2 + ((float)dig_P5)) / 16.0f;

    *temp_c = T;
    *pressure_pa = p;
    return true;
}
