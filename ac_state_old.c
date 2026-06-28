#include "ac_state.h"
#include <pico/time.h>
#include <pico/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 15 min
#define MAX_DUR_CLOSE_TARGET_US 15*60*1000*1000 
#define DEBOUNCE_DUR_US 1*60*1000*1000

static ac_state state;
static absolute_time_t last_state_change;
static float target_temp = 25.5, delta_t_s = 1, delta_t_m = 3,
  target_hum = 52.0, delta_h_s = 8, delta_h_m = 16;

void ac_state_init() {
  state = OPTIMAL_OFF;
  last_state_change = get_absolute_time();
}

bool is_state(ac_state s) { return (state == s); }

bool is_idle_state(ac_state s) {
  return (s == OPTIMAL_OFF || s == RESIDUAL_FAN);
}

ac_state set_state(ac_state s) {
  state = s;
  last_state_change = get_absolute_time();
  return state;
}

ac_state get_state() { return state; }

void get_state_str(ac_state s, char *str, const size_t str_len) {
  switch (s) {
  case A_BIT_HOT:
    strncpy(str, "A bit hot", str_len);
    break;
  case MEDIUM_HOT:
    strncpy(str, "Medium hot", str_len);
    break;
  case VERY_HOT:
    strncpy(str, "Very hot", str_len);
    break;
  case OPTIMAL_OFF:
    strncpy(str, "Optimal", str_len);
    break;
  case RESIDUAL_FAN:
    strncpy(str, "Residual", str_len);
    break;
  }
}

bool next_state(float temp, float hum) {
  // TODO: only handles positive deltas (cooling mode)
  const float delta_t = temp - target_temp, delta_h = hum - target_hum;
  int64_t since_last_change = absolute_time_diff_us(last_state_change, get_absolute_time());

  // Skip to debounce
  if (since_last_change < DEBOUNCE_DUR_US) return false;

  if (delta_t <= 0 && delta_h <= 0) {
    if (state == VERY_HOT || state == MEDIUM_HOT) {
      set_state(A_BIT_HOT);
      return true;
    } else if (state == A_BIT_HOT) {
      set_state(RESIDUAL_FAN);
      return true;
    } else if (state == RESIDUAL_FAN) {
      set_state(OPTIMAL_OFF);
      return true;
     } else printf("STATE: OPTIMAL OFF already set\n");
  } else if (delta_t < delta_t_s && delta_h < delta_h_s) {
    if (since_last_change >= MAX_DUR_CLOSE_TARGET_US) {
      if (!is_idle_state(state)) {
        set_state(RESIDUAL_FAN);
        return true;
      } else if (state == RESIDUAL_FAN) {
        set_state(OPTIMAL_OFF);
        return true;
      } else printf("STATE: OPTIMAL OFF already set\n");
    } else {
      if (state != A_BIT_HOT) {
        set_state(A_BIT_HOT);
        return true;
      } else printf("STATE: A BIT HOT already set\n");
    }
  } else if (delta_t < delta_t_s) {
    if (state != A_BIT_HOT) {
      set_state(A_BIT_HOT);
      return true;
    } else printf("STATE: A BIT HOT already set\n");
  } else if (delta_t < delta_t_m) {
    if (state != MEDIUM_HOT) {
      set_state(MEDIUM_HOT);
      return true;
    } else printf("STATE: MEDIUM HOT already set\n");
  } else {
    if (state != VERY_HOT) {
      set_state(VERY_HOT);
      return true;
    } else printf("STATE: VERY HOT already set\n");
  }
  return false;
}
