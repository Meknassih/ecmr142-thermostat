#include "aht20.h"
#include "pico/stdlib.h"

#define AHT20_I2C_ADDR       0x38
#define AHT20_CMD_INITIALIZE 0xBE
#define AHT20_CMD_MEASURE    0xAC
#define AHT20_CMD_STATUS     0x71
#define AHT20_STATUS_BUSY    0x80
#define AHT20_STATUS_CAL     0x08
#define AHT20_POWERUP_MS     40
#define AHT20_INIT_MS        10
#define AHT20_MEASURE_MS     80

static i2c_inst_t *i2c_port;

static void write_cmd(uint8_t *buf, int len) {
    i2c_write_blocking(i2c_port, AHT20_I2C_ADDR, buf, len, false);
}

static void read_bytes(uint8_t *buf, int len) {
    i2c_read_blocking(i2c_port, AHT20_I2C_ADDR, buf, len, false);
}

bool aht20_init(i2c_inst_t *i2c) {
    i2c_port = i2c;
    sleep_ms(AHT20_POWERUP_MS);

    uint8_t status;
    uint8_t status_cmd = AHT20_CMD_STATUS;
    i2c_write_blocking(i2c_port, AHT20_I2C_ADDR, &status_cmd, 1, true);
    read_bytes(&status, 1);

    if (!(status & AHT20_STATUS_CAL)) {
        uint8_t init_cmd[3] = {AHT20_CMD_INITIALIZE, 0x08, 0x00};
        write_cmd(init_cmd, 3);
        sleep_ms(AHT20_INIT_MS);
    }

    return true;
}

bool aht20_read(float *temp_c, float *humidity) {
    uint8_t measure_cmd[3] = {AHT20_CMD_MEASURE, 0x33, 0x00};
    write_cmd(measure_cmd, 3);
    sleep_ms(AHT20_MEASURE_MS);

    uint8_t d[7];
    read_bytes(d, 7);

    if (d[0] & AHT20_STATUS_BUSY) return false;

    uint32_t raw_h = ((uint32_t)d[1] << 12) | ((uint32_t)d[2] << 4) | (d[3] >> 4);
    uint32_t raw_t = (((uint32_t)d[3] & 0x0F) << 16) | ((uint32_t)d[4] << 8) | d[5];

    *humidity = (float)raw_h / 1048576.0f * 100.0f;
    *temp_c   = (float)raw_t / 1048576.0f * 200.0f - 50.0f;

    return true;
}
