#ifndef ECMR142_H
#define ECMR142_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FRAME_MAX_LEN 16
#define SIGNALS_COUNT 13

typedef struct {
  const char *name;
  const uint8_t *frame;
  const size_t frame_len;
} ecmr142_signal;

extern const uint8_t cold_22c_fan1[FRAME_MAX_LEN];
extern const uint8_t cold_22c_fan2[FRAME_MAX_LEN];
extern const uint8_t cold_22c_fan3[FRAME_MAX_LEN];
extern const uint8_t mode_dh[FRAME_MAX_LEN];
extern const uint8_t mode_fan[FRAME_MAX_LEN];
extern const uint8_t fan1[FRAME_MAX_LEN];
extern const uint8_t fan2[FRAME_MAX_LEN];
extern const uint8_t fan3[FRAME_MAX_LEN];
extern const uint8_t heat_18c_fan1[FRAME_MAX_LEN];
extern const uint8_t mode_smart[FRAME_MAX_LEN];
extern const uint8_t cold_off[FRAME_MAX_LEN];
extern const uint8_t cold_on[FRAME_MAX_LEN];
extern const uint8_t fan1_off[FRAME_MAX_LEN];

extern const ecmr142_signal signals[SIGNALS_COUNT];

bool ident_frame(const uint8_t *frame, const size_t frame_len, char *frame_name, const size_t name_max_len);
#endif
