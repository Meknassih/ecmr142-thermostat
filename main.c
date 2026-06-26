#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include <stdbool.h>
#include "hardware/i2c.h"

#define LED_PIN 5
#define BTN_PIN 15
#define OLED_SDA_PIN 10
#define OLED_SCL_PIN 11

void toggle_running_led(unsigned int gpio) {
    bool curr = cyw43_arch_gpio_get(gpio);
    if (curr) 
        cyw43_arch_gpio_put(gpio, 0);
    else
        cyw43_arch_gpio_put(gpio, 1);
}

int main() {
    gpio_init(BTN_PIN);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, 1);
    gpio_set_dir(BTN_PIN, 0);
    gpio_pull_down(BTN_PIN);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);
    i2c_init(i2c_default, 100 * 1000);

    int err = cyw43_arch_init();
    if (err) {
        reset_usb_boot(0, 0);
    }

    unsigned int tick=10;

    toggle_running_led(CYW43_WL_GPIO_LED_PIN);
    while (true) {
        bool btn = gpio_get(BTN_PIN);
        gpio_put(LED_PIN, btn);
        if (btn) {
            reset_usb_boot(0, 0);
        }

        sleep_ms(tick);
    }
}
