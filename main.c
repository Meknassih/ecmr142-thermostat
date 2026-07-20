#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "aht20.h"
#include "bmp280.h"
#include "ir_receiver.h"
#include "ir_emitter.h"
#include <stdio.h>
#include "ecmr142.h"
#include "ac_state.h"
#include "ac_strategy_economic.h"
#include "humidex.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define VLOG(fmt, ...) printf("[V] " fmt "\n", ##__VA_ARGS__)
#define VOK(label)     printf("[V] OK   %s\n", label)
#define VFAIL(label)   printf("[V] FAIL %s\n", label)
#else
#define VLOG(fmt, ...) ((void)0)
#define VOK(label)     ((void)0)
#define VFAIL(label)   ((void)0)
#endif

#define LED_PIN 1
#define BTN_PIN 15
#define OLED_SDA_PIN 4
#define OLED_SCL_PIN 5
#define IR_RCV_PIN 0
#define IR_EMIT_PIN 16

void toggle_running_led(unsigned int gpio) {
    bool curr = cyw43_arch_gpio_get(gpio);
    if (curr) 
        cyw43_arch_gpio_put(gpio, 0);
    else
        cyw43_arch_gpio_put(gpio, 1);
}

int main() {
    stdio_init_all();
    sleep_ms(1500); // Allow USB serial to connect
    VLOG("stdio initialized, boot log follows");

    VLOG("init BTN_PIN=%d LED_PIN=%d", BTN_PIN, LED_PIN);
    gpio_init(BTN_PIN);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, 1);
    gpio_set_dir(BTN_PIN, 0);
    gpio_pull_down(BTN_PIN);
    VOK("gpio BTN/LED");

    VLOG("init I2C OLED SDA=%d SCL=%d @400kHz", OLED_SDA_PIN, OLED_SCL_PIN);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);
    i2c_init(i2c0, 400 * 1000);
    VOK("i2c init");

    VLOG("I2C bus scan:");
    for (uint8_t a = 0x01; a < 0x7F; a++) {
        uint8_t zero = 0;
        int r = i2c_write_blocking(i2c0, a, &zero, 1, false);
        if (r >= 0) printf("I2C: 0x%02X ACK\n", a);
    }
    VOK("i2c scan");

    VLOG("cyw43_arch_init");
    int err = cyw43_arch_init();
    if (err) {
        VFAIL("cyw43_arch_init");
        printf("ERR: cyw43_arch_init returned %d\n", err);
        reset_usb_boot(0, 0);
    }
    VOK("cyw43_arch_init");

    VLOG("ssd1306_init");
    if (!ssd1306_init(i2c_default)) {
        VFAIL("ssd1306_init");
        printf("ERR: OLED init failed (no ACK at 0x%02X)\n", 0x3C);
    } else {
        VOK("ssd1306_init");
        ssd1306_clear();
        ssd1306_write_string(16, 12, "Started");
        if (ssd1306_show() < 0) {
            VFAIL("ssd1306_show");
            printf("ERR: OLED show NACKed\n");
        } else {
            VOK("ssd1306 clear, write, show");
        }
    }

    VLOG("aht20_init");
    if (!aht20_init(i2c_default)) {
        VFAIL("aht20_init");
        printf("WARN: AHT20 init failed\n");
    } else {
        VOK("aht20_init");
    }

    VLOG("bmp280_init");
    if (!bmp280_init(i2c_default)) {
        VFAIL("bmp280_init");
        printf("WARN: BMP280 init failed\n");
    } else {
        VOK("bmp280_init");
    }

    VLOG("ir_receiver_init pin=%d", IR_RCV_PIN);
    ir_receiver_init(IR_RCV_PIN);
    VOK("ir_receiver_init");

    VLOG("ir_emitter_init pin=%d", IR_EMIT_PIN);
    ir_emitter_init(IR_EMIT_PIN, 38);
    VOK("ir_emitter_init");

    VLOG("ac_state_init + strategy_economic");
    ac_state_init();
    ac_state_set_strategy(&strategy_economic);
    VOK("ac_state ready");

    unsigned int tick=10;
    unsigned long int tick_count=0;
    VLOG("entering main loop");


    float aht_t = 0.0f, aht_h = 0.0f;
    float bmp_t = 0.0f, bmp_p = 0.0f;
    float avg_t = 0.0f;
    float humidex = 0.0f;

    toggle_running_led(CYW43_WL_GPIO_LED_PIN);

    ac_state_init();
    ac_state_set_strategy(&strategy_economic);

    char *addr_str = calloc(64, sizeof(char)), *cmd_str = calloc(64, sizeof(char));
    uint8_t addr = 0;
    uint8_t cmd = 0;
    while (true) {
        char s_line[17];
        ac_state_str(ac_state_get(), s_line, sizeof(s_line));
        ssd1306_clear();
        ssd1306_write_string(8, 0, s_line);

        if (tick_count % 100 == 0) {
            if (aht20_read(&aht_t, &aht_h)) {
                printf("AHT20: %.1fC  %.1f%%RH\n", aht_t, aht_h);
            }
            if (bmp280_read(&bmp_t, &bmp_p)) {
                printf("BMP280: %.1fC  %.0fPa\n", bmp_t, bmp_p);
            }
            avg_t = (aht_t + bmp_t) / 2;
            humidex = humidex_compute(avg_t, aht_h);

            ac_state_evaluate(avg_t, aht_h);
            if (ac_state_changed()) {
                switch (ac_state_get()) {
                case AC_COOL_LOW:
                    ir_emitter_send_signal(IR_EMIT_PIN, "cold_22c_fan1");
                    break;
                case AC_COOL_MED:
                    ir_emitter_send_signal(IR_EMIT_PIN, "cold_22c_fan2");
                    break;
                case AC_COOL_HIGH:
                    ir_emitter_send_signal(IR_EMIT_PIN, "cold_22c_fan3");
                    break;
                case AC_OFF:
                    ir_emitter_send_signal(IR_EMIT_PIN, "cold_off");
                    break;
                case AC_FAN:
                    ir_emitter_send_signal(IR_EMIT_PIN, "fan1");
                    break;
                }
            }
        }

        // Write sensor values to OLED
        char t_h_line[17], hdx_line[17];
        snprintf(t_h_line, sizeof(t_h_line), "%.1fC %.1f%%",
                 avg_t, aht_h);
        ssd1306_write_string(0, 16, t_h_line);
        snprintf(hdx_line, sizeof(hdx_line), "H %.1f", humidex);
        ssd1306_write_string(0, 24, hdx_line);

        // IR raw capture
        /* uint32_t durations[400];
        ir_receiver_capture_raw(IR_RCV_PIN, durations, 400, 1000);
        ir_receiver_print_raw(durations, 400); */
        
        // IR scan
        /* uint8_t bytes[FRAME_MAX_LEN];
        int bits = ir_receiver_scan_frame(IR_RCV_PIN, bytes, 16, 1000);
        if (bits > 0) {
            const int byte_count = (bits + 7) / 8, hex_str_len = FRAME_MAX_LEN*7;
            const char hex_str[hex_str_len];
            frame_to_hex_str(bytes, byte_count, (char*)hex_str, hex_str_len);
            printf("Decoded %d bits:\n", bits);
            printf("\t%s\n", hex_str);

            char signal_name32[32];
            if (ident_frame(bytes, byte_count, signal_name32, 32)) {
                printf("\tIdent: %s\n", signal_name32);
            }
        } */

        // Reset to flash mode button
        bool btn = gpio_get(BTN_PIN);
        gpio_put(LED_PIN, btn);
        if (btn) {
            sleep_ms(1000);
            ssd1306_clear();
            ssd1306_show();
            reset_usb_boot(0, 0);
        }

        ssd1306_show();
        sleep_ms(tick);
        tick_count++;
    }
}
