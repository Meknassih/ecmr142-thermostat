#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>
#include "hardware/i2c.h"

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 32

void ssd1306_init(i2c_inst_t *i2c);
void ssd1306_clear(void);
void ssd1306_show(void);
void ssd1306_fill(bool on);
void ssd1306_set_pixel(int x, int y, bool on);
void ssd1306_write_string(int x, int y, const char *str);

#endif