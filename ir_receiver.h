#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <stdint.h>
#include <stdbool.h>

void ir_receiver_init(unsigned int pin);

int  ir_receiver_capture_raw(unsigned int pin, uint32_t *durations, int max_durations, uint32_t timeout_ms);
void ir_receiver_print_raw(const uint32_t *durations, int count);
bool ir_receiver_decode_nec(const uint32_t *durations, int count, uint8_t *addr, uint8_t *cmd);
bool ir_receiver_scan_nec(unsigned int pin, uint8_t *addr, uint8_t *cmd, uint32_t timeout_ms);
bool ir_receiver_decode_raw_nec(const uint32_t *durations, int count, uint32_t *raw_data);
int  ir_receiver_decode_frame(const uint32_t *durations, int count, uint8_t *bytes, int max_bytes);
int  ir_receiver_scan_frame(unsigned int pin, uint8_t *bytes, int max_bytes, uint32_t timeout_ms);

#endif