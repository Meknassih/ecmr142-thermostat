#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "ir_receiver.h"
#include "ir_emitter.h"
#include <stdio.h>
#include "ecmr142.h"

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
    ir_emitter_init(IR_EMIT_PIN, 38);

    unsigned int tick=10;
    unsigned long int tick_count=0;

    toggle_running_led(CYW43_WL_GPIO_LED_PIN);

    char *addr_str = calloc(64, sizeof(char)), *cmd_str = calloc(64, sizeof(char));
    uint8_t addr = 0;
    uint8_t cmd = 0;
    while (true) {
        ssd1306_clear();
        ssd1306_write_string(2, 0, "Running...");

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

        // IR Emit
        if (tick_count%600 == 0) {
            ecmr142_signal sig = {0};
            if(!get_signal_by_name("cold_22c_fan1", strlen("cold_22c_fan1")+1, &sig)) {
                printf("WARN: Failed to get signal `cold_22c_fan1`\n");
            }
            ir_emitter_send_bytes(IR_EMIT_PIN, sig.frame, sig.frame_len);
            char frame_str[FRAME_MAX_LEN*7] = "\0";
            frame_to_hex_str(sig.frame, sig.frame_len, frame_str, FRAME_MAX_LEN*7);
            printf("\nINFO: Emitted signal '%s' (%lu bytes)\n", sig.name, sig.frame_len);
            printf("%s\n", frame_str);
        } else if (tick_count%600 == 200) {
            ecmr142_signal sig = {0};
            if(!get_signal_by_name("cold_22c_fan2", strlen("cold_22c_fan2")+1, &sig)) {
                printf("WARN: Failed to get signal `cold_22c_fan2`\n");
            }
            ir_emitter_send_bytes(IR_EMIT_PIN, sig.frame, sig.frame_len);
            char frame_str[FRAME_MAX_LEN*7] = "\0";
            frame_to_hex_str(sig.frame, sig.frame_len, frame_str, FRAME_MAX_LEN*7);
            printf("\nINFO: Emitted signal '%s' (%lu bytes)\n", sig.name, sig.frame_len);
            printf("%s\n", frame_str);
        } else if (tick_count%600 == 400) {
            ecmr142_signal sig = {0};
            if(!get_signal_by_name("cold_22c_fan3", strlen("cold_22c_fan3")+1, &sig)) {
                printf("WARN: Failed to get signal `cold_22c_fan3`\n");
            }
            ir_emitter_send_bytes(IR_EMIT_PIN, sig.frame, sig.frame_len);
            char frame_str[FRAME_MAX_LEN*7] = "\0";
            frame_to_hex_str(sig.frame, sig.frame_len, frame_str, FRAME_MAX_LEN*7);
            printf("\nINFO: Emitted signal '%s' (%lu bytes)\n", sig.name, sig.frame_len);
            printf("%s\n", frame_str);
        }

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
