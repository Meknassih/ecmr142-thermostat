#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include <stdbool.h>
#include <stdlib.h>
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "ir_receiver.h"
#include <stdio.h>
#include "ecmr142.h"

#define LED_PIN 5
#define BTN_PIN 15
#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9
#define IR_RCV_PIN 7

void toggle_running_led(unsigned int gpio) {
    bool curr = cyw43_arch_gpio_get(gpio);
    if (curr) 
        cyw43_arch_gpio_put(gpio, 0);
    else
        cyw43_arch_gpio_put(gpio, 1);
}

int main() {
    stdio_init_all();
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

    ir_receiver_init(IR_RCV_PIN);

    unsigned int tick=10;

    toggle_running_led(CYW43_WL_GPIO_LED_PIN);

    char *addr_str = calloc(64, sizeof(char)), *cmd_str = calloc(64, sizeof(char));
    uint8_t addr = 0;
    uint8_t cmd = 0;
    while (true) {
        ssd1306_clear();
        ssd1306_write_string(2, 0, "Running...");

        // IR scan
        uint8_t bytes[FRAME_MAX_LEN];
        int bits = ir_receiver_scan_frame(IR_RCV_PIN, bytes, 16, 1000);
        if (bits > 0) {
            int byte_count = (bits + 7) / 8;
            printf("Decoded %d bits:\n", bits);
            printf("\t");
            for (int i = 0; i < byte_count; i++) {
                if (i == ((bits + 7) / 8) - 1)
                    printf("0x%02X\n", bytes[i]);
                else
                    printf("0x%02X, ", bytes[i]);
            }

            char signal_name32[32];
            if (ident_frame(bytes, byte_count, signal_name32, 32)) {
                printf("\tIdent: %s\n", signal_name32);
            }
        }
        
        // Debug edge count
        /* uint32_t raw[400];
        int n = ir_receiver_capture_raw(IR_RCV_PIN, raw, 400, 1000);
        char *str1 = calloc(24, sizeof(char));
        char *str2 = calloc(24, sizeof(char));
        char *str3 = calloc(24, sizeof(char));
        sprintf(str1, "%d,%d,%d,%d",raw[0],raw[1],raw[2],raw[3]);
        sprintf(str2, "%d,%d,%d,%d",raw[4],raw[5],raw[6],raw[7]);
        sprintf(str3, "%d,%d,%d,%d",raw[8],raw[9],raw[10],raw[11]);
        ssd1306_write_string(2, 10, str1);
        ssd1306_write_string(2, 18, str2);
        ssd1306_write_string(2, 25, str3);
        ir_receiver_print_raw(raw, n); */
        
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
    }
}
