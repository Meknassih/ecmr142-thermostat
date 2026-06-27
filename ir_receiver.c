#include "ir_receiver.h"
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include <stdio.h>

#define IR_MAX_EDGES 400

#define NEC_LEADER_LOW_MIN   2500
#define NEC_LEADER_LOW_MAX   4000
#define NEC_LEADER_HIGH_MIN  1200
#define NEC_LEADER_HIGH_MAX  2200
#define NEC_BIT_LOW_MIN      300
#define NEC_BIT_LOW_MAX      600
#define NEC_BIT_HIGH_0_MIN   250
#define NEC_BIT_HIGH_0_MAX   550
#define NEC_BIT_HIGH_1_MIN   900
#define NEC_BIT_HIGH_1_MAX   1600
#define NEC_STOP_MIN         300
#define NEC_STOP_MAX         600

static volatile uint64_t ir_edge_times[IR_MAX_EDGES];
static volatile int ir_edge_count;
static volatile bool ir_overflow;

static void ir_edge_isr(unsigned int gpio, uint32_t events) {
    (void)gpio;
    (void)events;
    if (ir_edge_count < IR_MAX_EDGES) {
        ir_edge_times[ir_edge_count++] = time_us_64();
    } else {
        ir_overflow = true;
    }
}

void ir_receiver_init(unsigned int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);

    gpio_set_irq_callback(ir_edge_isr);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

int ir_receiver_capture_raw(unsigned int pin, uint32_t *durations, int max_durations, uint32_t timeout_ms) {
    ir_edge_count = 0;
    ir_overflow = false;

    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    while (!time_reached(deadline) && ir_edge_count < max_durations + 1) {
        tight_loop_contents();
    }

    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

    int n = ir_edge_count;
    if (n < 2) return 0;

    int d = 0;
    for (int i = 1; i < n && d < max_durations; i++) {
        durations[d++] = (uint32_t)(ir_edge_times[i] - ir_edge_times[i - 1]);
    }

    return d;
}

void ir_receiver_print_raw(const uint32_t *durations, int count) {
    printf("IR raw timings (%d edges):\n", count);
    for (int i = 0; i < count; i++) {
        printf("  [%3d] %5lu us %s\n", i, (unsigned long)durations[i],
               (i % 2 == 0) ? "LOW " : "HIGH");
    }
}

bool ir_receiver_decode_nec(const uint32_t *durations, int count,
                            uint8_t *addr, uint8_t *cmd) {
    for (int idx = 0; idx + 67 <= count; idx++) {
        if (durations[idx] < NEC_LEADER_LOW_MIN ||
            durations[idx] > NEC_LEADER_LOW_MAX) continue;
        if (durations[idx + 1] < NEC_LEADER_HIGH_MIN ||
            durations[idx + 1] > NEC_LEADER_HIGH_MAX) continue;

        uint32_t data = 0;
        bool valid = true;
        for (int i = 0; i < 32; i++) {
            uint32_t low  = durations[idx + 2 + i * 2];
            uint32_t high = durations[idx + 2 + i * 2 + 1];
            if (low < NEC_BIT_LOW_MIN || low > NEC_BIT_LOW_MAX) {
                valid = false;
                break;
            }
            if (high >= NEC_BIT_HIGH_1_MIN && high <= NEC_BIT_HIGH_1_MAX) {
                data |= (1u << i);
            } else if (high < NEC_BIT_HIGH_0_MIN ||
                       high > NEC_BIT_HIGH_0_MAX) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;

        uint32_t stop = durations[idx + 2 + 32 * 2];
        if (stop < NEC_STOP_MIN || stop > NEC_STOP_MAX) continue;

        *addr = (uint8_t)(data & 0xFF);
        *cmd  = (uint8_t)((data >> 16) & 0xFF);
        return true;
    }
    return false;
}

bool ir_receiver_scan_nec(unsigned int pin, uint8_t *addr, uint8_t *cmd,
                          uint32_t timeout_ms) {
    uint32_t raw[IR_MAX_EDGES];
    int n = ir_receiver_capture_raw(pin, raw, IR_MAX_EDGES, timeout_ms);
    if (n < 67) return false;
    return ir_receiver_decode_nec(raw, n, addr, cmd);
}

bool ir_receiver_decode_raw_nec(const uint32_t *durations, int count,
                                uint32_t *raw_data) {
    for (int idx = 0; idx + 67 <= count; idx++) {
        if (durations[idx] < NEC_LEADER_LOW_MIN ||
            durations[idx] > NEC_LEADER_LOW_MAX) continue;
        if (durations[idx + 1] < NEC_LEADER_HIGH_MIN ||
            durations[idx + 1] > NEC_LEADER_HIGH_MAX) continue;

        uint32_t data = 0;
        bool valid = true;
        for (int i = 0; i < 32; i++) {
            uint32_t low  = durations[idx + 2 + i * 2];
            uint32_t high = durations[idx + 2 + i * 2 + 1];
            if (low < NEC_BIT_LOW_MIN || low > NEC_BIT_LOW_MAX) {
                valid = false;
                break;
            }
            if (high >= NEC_BIT_HIGH_1_MIN && high <= NEC_BIT_HIGH_1_MAX) {
                data |= (1u << i);
            } else if (high < NEC_BIT_HIGH_0_MIN ||
                       high > NEC_BIT_HIGH_0_MAX) {
                valid = false;
                break;
            }
        }
        if (!valid) continue;

        uint32_t stop = durations[idx + 2 + 32 * 2];
        if (stop < NEC_STOP_MIN || stop > NEC_STOP_MAX) continue;

        *raw_data = data;
        return true;
    }
    return false;
}

#define NEC_FRAME_GAP_MIN 2000

int ir_receiver_decode_frame(const uint32_t *durations, int count,
                             uint8_t *bytes, int max_bytes) {
    for (int idx = 0; idx + 4 <= count; idx++) {
        if (durations[idx] < NEC_LEADER_LOW_MIN ||
            durations[idx] > NEC_LEADER_LOW_MAX) continue;
        if (durations[idx + 1] < NEC_LEADER_HIGH_MIN ||
            durations[idx + 1] > NEC_LEADER_HIGH_MAX) continue;

        int byte_idx = 0;
        int bit_idx = 0;
        int dur_idx = idx + 2;

        while (dur_idx + 1 < count) {
            uint32_t low  = durations[dur_idx];
            uint32_t high = durations[dur_idx + 1];

            if (high >= NEC_FRAME_GAP_MIN) break;

            if (low < NEC_BIT_LOW_MIN || low > NEC_BIT_LOW_MAX) {
                bit_idx = 0;
                break;
            }

            if (byte_idx >= max_bytes) break;

            int bit = 0;
            if (high >= NEC_BIT_HIGH_1_MIN && high <= NEC_BIT_HIGH_1_MAX) {
                bit = 1;
            } else if (high < NEC_BIT_HIGH_0_MIN ||
                       high > NEC_BIT_HIGH_0_MAX) {
                bit_idx = 0;
                break;
            }

            if (bit_idx == 0 && byte_idx < max_bytes)
                bytes[byte_idx] = 0;

            if (bit)
                bytes[byte_idx] |= (1u << bit_idx);

            bit_idx++;
            if (bit_idx >= 8) {
                byte_idx++;
                bit_idx = 0;
            }

            dur_idx += 2;
        }

        if (byte_idx > 0 || bit_idx > 0) {
            int total_bits = byte_idx * 8 + bit_idx;
            return total_bits;
        }
    }
    return -1;
}

int ir_receiver_scan_frame(unsigned int pin, uint8_t *bytes,
                           int max_bytes, uint32_t timeout_ms) {
    uint32_t raw[IR_MAX_EDGES];
    int n = ir_receiver_capture_raw(pin, raw, IR_MAX_EDGES, timeout_ms);
    if (n < 4) return -1;
    return ir_receiver_decode_frame(raw, n, bytes, max_bytes);
}
