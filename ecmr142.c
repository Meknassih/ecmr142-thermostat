#include "ecmr142.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

const uint8_t
    cold_22c_fan1[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03,
                                    0x09, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x7F},
    cold_22c_fan2[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03,
                                    0x09, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x80},
    cold_22c_fan3[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03,
                                    0x09, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x82},
    cold_22c_fana[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03,
                                    0x09, 0x38, 0x00, 0x00, 0x00, 0x00, 0x7D},
    mode_dh[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x02,
                              0x08, 0x38, 0x00, 0x00, 0x00, 0x00, 0x7B},
    fan1[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x07,
                           0x08, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x82},
    fan2[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x07,
                           0x08, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x83},
    fan3[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x07,
                           0x08, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x85},
    heat_18c_fan1[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x01,
                                    0x0A, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x81},
    mode_smart[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x08,
                                 0x08, 0x38, 0x00, 0x00, 0x00, 0x00, 0x81},
    cold_off[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x03,
                               0x09, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x7B},
    cold_on[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03,
                              0x09, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x7F},
    fan1_off[FRAME_MAX_LEN] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x07,
                               0x08, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x7E};

const ecmr142_signal signals[SIGNALS_COUNT] = {
    {"cold_22c_fan1", cold_22c_fan1, 14},
    {"cold_22c_fan2", cold_22c_fan2, 14},
    {"cold_22c_fan3", cold_22c_fan3, 14},
    {"cold_22c_fana", cold_22c_fana, 14},
    {"mode_dh", mode_dh, 14},
    {"fan1", fan1, 14},
    {"fan2", fan2, 14},
    {"fan3", fan3, 14},
    {"heat_18c_fan1", heat_18c_fan1, 14},
    {"mode_smart", mode_smart, 14},
    {"cold_off", cold_off, 14},
    {"cold_on", cold_on, 14},
    {"fan1_off", fan1_off, 14},
};

bool ident_frame(const uint8_t *frame, const size_t frame_len, char *frame_name, const size_t name_max_len) {
  if (frame_len > FRAME_MAX_LEN) {
    printf("WARN: Attempting to ident frame with length > FRAME_MAX_LEN(%d)\n", FRAME_MAX_LEN);
    return false;
  }

  bool has_inequality = false;
  for (int i=0; i<SIGNALS_COUNT; i++) {
    if (signals[i].frame_len != frame_len)
      continue;

    for (int j=0; j<FRAME_MAX_LEN; j++) {
      if (signals[i].frame[j] != frame[j]) {
        has_inequality = true;
        break;
      }
    }

    if (has_inequality) {
      // Reset flag and go next
      has_inequality = false;
    } else {
      // Copy name and return
      strncpy(frame_name, signals[i].name, name_max_len);
      return true;
    }
  }

  return false;
}

void frame_to_hex_str(const uint8_t *frame, const size_t frame_len, char* str, const size_t str_max_len) {
  str[0] = '\0';
  const unsigned short byte_str_len = 7;
  for (int i = 0; i < frame_len; i++) {
    char byte_str[byte_str_len]; // `0xAA, \0`
    if (i == (frame_len - 1))
      snprintf(byte_str, byte_str_len, "0x%02X", frame[i]);
    else
      snprintf(byte_str, byte_str_len, "0x%02X, ", frame[i]);

    strcat(str, byte_str);
  }
}

void clone_signal(ecmr142_signal *dest, const ecmr142_signal *src) {
  dest->name = src->name;
  // printf("\t\tcopied %s -> %s\n", src->name, dest->name);
  dest->frame = src->frame;
  dest->frame_len = src->frame_len;
  const unsigned short frame_str_len = FRAME_MAX_LEN*7;
  char src_frame_str[frame_str_len], dest_frame_str[frame_str_len];
  frame_to_hex_str(src->frame, src->frame_len, src_frame_str, frame_str_len);
  frame_to_hex_str(dest->frame, dest->frame_len, dest_frame_str, frame_str_len);
  // printf("\tcopied\n\t\t%s ->\n\t\t%s\n", src_frame_str, dest_frame_str);
}

bool get_signal_by_name(const char* signal_name, const size_t name_max_len, ecmr142_signal *signal) {
  for (int i=0; i<SIGNALS_COUNT; i++) {
    const unsigned int res = strncmp(signals[i].name, signal_name, name_max_len);
    // printf("INFO: `%s` == `%s`? %u\n", signals[i].name, signal_name, res);
    if (res == 0) {
      // printf("\tFound signal %s\n", signals[i].name);
      clone_signal(signal, &(signals[i]));
      return true;
    }
  }

  return false;
}

