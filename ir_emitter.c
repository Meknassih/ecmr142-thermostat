#include "ir_emitter.h"
#include "ecmr142.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include <stdio.h>
#include <string.h>

static unsigned int ir_emitter_slice;

#define MITSUBISHI_HDR_MARK   3400
#define MITSUBISHI_HDR_SPACE  1750
#define MITSUBISHI_BIT_MARK   450
#define MITSUBISHI_ONE_SPACE  1300
#define MITSUBISHI_ZERO_SPACE 420
#define MITSUBISHI_RPT_MARK   440
#define MITSUBISHI_RPT_SPACE  17100
#define MITSUBISHI_FRAME_GAP  40000

#define NEC_LEADER_LOW  MITSUBISHI_HDR_MARK
#define NEC_LEADER_HIGH MITSUBISHI_HDR_SPACE
#define NEC_BIT_LOW     MITSUBISHI_BIT_MARK
#define NEC_BIT_HIGH_0  MITSUBISHI_ZERO_SPACE
#define NEC_BIT_HIGH_1  MITSUBISHI_ONE_SPACE
#define NEC_STOP        MITSUBISHI_BIT_MARK
#define NEC_REPEAT_GAP  MITSUBISHI_RPT_SPACE
#define NEC_FRAME_GAP   MITSUBISHI_FRAME_GAP

void ir_emitter_init(unsigned int pin, unsigned int carrier_khz) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    ir_emitter_slice = pwm_gpio_to_slice_num(pin);
    unsigned int channel = pwm_gpio_to_channel(pin);

    uint32_t sys_clk = clock_get_hz(clk_sys);
    const uint32_t wrap = 255;
    float divider = (float)sys_clk / (carrier_khz * 1000.0f * (wrap + 1));
    if (divider < 1.0f) divider = 1.0f;
    if (divider > 255.0f) divider = 255.0f;

    pwm_set_clkdiv(ir_emitter_slice, divider);
    pwm_set_wrap(ir_emitter_slice, wrap);
    pwm_set_chan_level(ir_emitter_slice, channel, (wrap + 1) / 2);
    pwm_set_enabled(ir_emitter_slice, false);
}

void ir_emitter_carrier_on(unsigned int pin) {
    (void)pin;
    pwm_set_enabled(ir_emitter_slice, true);
}

void ir_emitter_carrier_off(unsigned int pin) {
    (void)pin;
    pwm_set_enabled(ir_emitter_slice, false);
}

void ir_emitter_mark(unsigned int pin, uint32_t us) {
    ir_emitter_carrier_on(pin);
    sleep_us(us);
    ir_emitter_carrier_off(pin);
}

void ir_emitter_space(unsigned int pin, uint32_t us) {
    ir_emitter_carrier_off(pin);
    sleep_us(us);
}

static void ir_emitter_send_byte(unsigned int pin, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ir_emitter_mark(pin, NEC_BIT_LOW);
        if (byte & (1u << i)) {
            ir_emitter_space(pin, NEC_BIT_HIGH_1);
        } else {
            ir_emitter_space(pin, NEC_BIT_HIGH_0);
        }
    }
}

void ir_emitter_send_nec(unsigned int pin, uint8_t addr, uint8_t cmd) {
    ir_emitter_mark(pin, NEC_LEADER_LOW);
    ir_emitter_space(pin, NEC_LEADER_HIGH);

    ir_emitter_send_byte(pin, addr);
    ir_emitter_send_byte(pin, ~addr & 0xFF);
    ir_emitter_send_byte(pin, cmd);
    ir_emitter_send_byte(pin, ~cmd & 0xFF);

    ir_emitter_mark(pin, NEC_STOP);
    ir_emitter_space(pin, NEC_FRAME_GAP);
}

bool ir_emitter_send_nec_repeat(unsigned int pin, uint8_t addr, uint8_t cmd) {
    static uint8_t last_addr = 0xFF;
    static uint8_t last_cmd = 0xFF;
    static absolute_time_t last_time = {0};

    absolute_time_t now = get_absolute_time();
    uint64_t elapsed = absolute_time_diff_us(last_time, now);

    if (addr == last_addr && cmd == last_cmd && elapsed < 200000) {
        ir_emitter_mark(pin, NEC_LEADER_LOW);
        ir_emitter_space(pin, NEC_REPEAT_GAP);
        ir_emitter_mark(pin, NEC_STOP);
        ir_emitter_space(pin, NEC_FRAME_GAP);
        return true;
    }

    ir_emitter_send_nec(pin, addr, cmd);
    last_addr = addr;
    last_cmd = cmd;
    last_time = now;
    return false;
}

void ir_emitter_send_raw(unsigned int pin, const uint32_t *durations, int count) {
    for (int i = 0; i < count; i++) {
        if (i % 2 == 0) {
            ir_emitter_mark(pin, durations[i]);
        } else {
            ir_emitter_space(pin, durations[i]);
        }
    }
    ir_emitter_carrier_off(pin);
}

void ir_emitter_send_bytes(unsigned int pin, const uint8_t *bytes, int count) {
    ir_emitter_mark(pin, NEC_LEADER_LOW);
    ir_emitter_space(pin, NEC_LEADER_HIGH);

    for (int i = 0; i < count; i++) {
        ir_emitter_send_byte(pin, bytes[i]);
    }

    ir_emitter_mark(pin, NEC_STOP);
    ir_emitter_space(pin, NEC_FRAME_GAP);
}

void ir_emitter_send_mitsubishi(unsigned int pin, const uint8_t *bytes, int count) {
    for (int frame = 0; frame < 2; frame++) {
        ir_emitter_mark(pin, MITSUBISHI_HDR_MARK);
        ir_emitter_space(pin, MITSUBISHI_HDR_SPACE);

        for (int i = 0; i < count; i++) {
            ir_emitter_send_byte(pin, bytes[i]);
        }

        ir_emitter_mark(pin, MITSUBISHI_RPT_MARK);

        if (frame == 0) {
            ir_emitter_space(pin, MITSUBISHI_RPT_SPACE);
        } else {
            ir_emitter_space(pin, MITSUBISHI_FRAME_GAP);
        }
    }
}

void ir_emitter_send_signal(unsigned int pin, char *signal_name) {
    ecmr142_signal sig = {0};
    if(!get_signal_by_name(signal_name, strlen(signal_name)+1, &sig)) {
        printf("WARN: Failed to get signal `%s`\n", signal_name);
    }
    ir_emitter_send_bytes(pin, sig.frame, sig.frame_len);
    char frame_str[FRAME_MAX_LEN*7] = "\0";
    frame_to_hex_str(sig.frame, sig.frame_len, frame_str, FRAME_MAX_LEN*7);
    printf("\nINFO: Emitted signal '%s' (%u bytes)\n", sig.name, sig.frame_len);
    printf("%s\n", frame_str);
}
