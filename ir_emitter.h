#ifndef IR_EMITTER_H
#define IR_EMITTER_H

#include <stdint.h>
#include <stdbool.h>

void ir_emitter_init(unsigned int pin, unsigned int carrier_khz);

void ir_emitter_send_nec(unsigned int pin, uint8_t addr, uint8_t cmd);
bool ir_emitter_send_nec_repeat(unsigned int pin, uint8_t addr, uint8_t cmd);

void ir_emitter_send_raw(unsigned int pin, const uint32_t *durations, int count);
void ir_emitter_send_bytes(unsigned int pin, const uint8_t *bytes, int count);
void ir_emitter_send_mitsubishi(unsigned int pin, const uint8_t *bytes, int count);
void ir_emitter_send_signal(unsigned int pin, char *signal_name);

void ir_emitter_carrier_on(unsigned int pin);
void ir_emitter_carrier_off(unsigned int pin);
void ir_emitter_mark(unsigned int pin, uint32_t us);
void ir_emitter_space(unsigned int pin, uint32_t us);

#endif
