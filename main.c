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

#define LED_PIN 5
#define BTN_PIN 15
#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9
#define IR_RCV_PIN 7
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
    gpio_init(BTN_PIN);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, 1);
    gpio_set_dir(BTN_PIN, 0);
    gpio_pull_down(BTN_PIN);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);
    i2c_init(i2c_default, 400 * 1000);

    int err = cyw43_arch_init();
    if (err) {
        reset_usb_boot(0, 0);
    }

    ssd1306_init(i2c_default);
    ssd1306_clear();
    ssd1306_write_string(16, 12, "Started");
    ssd1306_show();

    if (!aht20_init(i2c_default)) {
        printf("WARN: AHT20 init failed\n");
    }
    if (!bmp280_init(i2c_default)) {
        printf("WARN: BMP280 init failed\n");
    }

    ir_receiver_init(IR_RCV_PIN);
    ir_emitter_init(IR_EMIT_PIN, 38);

    unsigned int tick=10;
    unsigned long int tick_count=0;

    float aht_t = 0.0f, aht_h = 0.0f;
    float bmp_t = 0.0f, bmp_p = 0.0f;
    float avg_t = 0.0f;

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
        char t_h_line[17], p_line[17];
        snprintf(t_h_line, sizeof(t_h_line), "%.1fC %.1f%%",
                 avg_t, aht_h);
        ssd1306_write_string(0, 16, t_h_line);
        snprintf(p_line, sizeof(p_line), "%.0fPa", bmp_p);
        ssd1306_write_string(0, 24, p_line);

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
