#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ssd1306.h"
#include "ssd1306_font.h"

#define SSD1306_I2C_ADDR           0x3C

#define SSD1306_SET_MEM_MODE       0x20
#define SSD1306_SET_COL_ADDR       0x21
#define SSD1306_SET_PAGE_ADDR      0x22
#define SSD1306_SET_DISP_START_LINE 0x40
#define SSD1306_SET_CONTRAST       0x81
#define SSD1306_SET_CHARGE_PUMP    0x8D
#define SSD1306_SET_SEG_REMAP      0xA0
#define SSD1306_SET_ENTIRE_ON      0xA4
#define SSD1306_SET_ALL_ON         0xA5
#define SSD1306_SET_NORM_DISP      0xA6
#define SSD1306_SET_INV_DISP       0xA7
#define SSD1306_SET_MUX_RATIO      0xA8
#define SSD1306_SET_DISP           0xAE
#define SSD1306_SET_COM_OUT_DIR    0xC0
#define SSD1306_SET_DISP_OFFSET    0xD3
#define SSD1306_SET_DISP_CLK_DIV   0xD5
#define SSD1306_SET_PRECHARGE      0xD9
#define SSD1306_SET_COM_PIN_CFG    0xDA
#define SSD1306_SET_VCOM_DESEL     0xDB
#define SSD1306_SET_SCROLL         0x2E

#define SSD1306_PAGE_HEIGHT        8
#define SSD1306_NUM_PAGES          (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN            (SSD1306_NUM_PAGES * SSD1306_WIDTH)

static i2c_inst_t *i2c_port;
static uint8_t framebuf[SSD1306_BUF_LEN];

static void send_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_port, SSD1306_I2C_ADDR, buf, 2, false);
}

static void send_cmd_list(uint8_t *cmds, int num) {
    for (int i = 0; i < num; i++)
        send_cmd(cmds[i]);
}

static void send_buf(uint8_t *buf, int len) {
    uint8_t *temp = malloc(len + 1);
    temp[0] = 0x40;
    memcpy(temp + 1, buf, len);
    i2c_write_blocking(i2c_port, SSD1306_I2C_ADDR, temp, len + 1, false);
    free(temp);
}

static int get_font_index(uint8_t ch) {
    if (ch >= 'A' && ch <= 'Z') return (ch - 'A' + 1) * 8;
    if (ch >= '0' && ch <= '9') return (ch - '0' + 27) * 8;
    return 0;
}

void ssd1306_init(i2c_inst_t *i2c) {
    i2c_port = i2c;

    uint8_t cmds[] = {
        SSD1306_SET_DISP,
        SSD1306_SET_MEM_MODE, 0x00,
        SSD1306_SET_DISP_START_LINE,
        SSD1306_SET_SEG_REMAP | 0x01,
        SSD1306_SET_MUX_RATIO, SSD1306_HEIGHT - 1,
        SSD1306_SET_COM_OUT_DIR | 0x08,
        SSD1306_SET_DISP_OFFSET, 0x00,
        SSD1306_SET_COM_PIN_CFG, 0x02,
        SSD1306_SET_DISP_CLK_DIV, 0x80,
        SSD1306_SET_PRECHARGE, 0xF1,
        SSD1306_SET_VCOM_DESEL, 0x30,
        SSD1306_SET_CONTRAST, 0xFF,
        SSD1306_SET_ENTIRE_ON,
        SSD1306_SET_NORM_DISP,
        SSD1306_SET_CHARGE_PUMP, 0x14,
        SSD1306_SET_SCROLL | 0x00,
        SSD1306_SET_DISP | 0x01,
    };

    send_cmd_list(cmds, sizeof(cmds));
    ssd1306_clear();
    ssd1306_show();
}

void ssd1306_clear(void) {
    memset(framebuf, 0, SSD1306_BUF_LEN);
}

void ssd1306_fill(bool on) {
    memset(framebuf, on ? 0xFF : 0x00, SSD1306_BUF_LEN);
}

void ssd1306_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;

    int byte_idx = (y / 8) * SSD1306_WIDTH + x;
    if (on)
        framebuf[byte_idx] |= 1 << (y % 8);
    else
        framebuf[byte_idx] &= ~(1 << (y % 8));
}

void ssd1306_show(void) {
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR, 0, SSD1306_WIDTH - 1,
        SSD1306_SET_PAGE_ADDR, 0, SSD1306_NUM_PAGES - 1
    };
    send_cmd_list(cmds, sizeof(cmds));
    send_buf(framebuf, SSD1306_BUF_LEN);
}

void ssd1306_write_string(int x, int y, const char *str) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8) return;

    while (*str) {
        if (x > SSD1306_WIDTH - 8) break;

        uint8_t ch = (uint8_t)toupper(*str);
        int fi = get_font_index(ch);
        int fb_idx = (y / 8) * SSD1306_WIDTH + x;

        for (int i = 0; i < 8; i++)
            framebuf[fb_idx++] = font[fi + i];

        str++;
        x += 8;
    }
}