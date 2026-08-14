#include "ac_strategy_time_interval.h"
#include "ac_state.h"
#include "humidex.h"
#include <pico/time.h>
#include <stdio.h>

#define COOL_DUR_US           (60ull * 60 * 1000 * 1000)
#define COOL_DUR_MIN_US       (15ull * 60 * 1000 * 1000)
#define COOL_DUR_MAX_US       (180ull * 60 * 1000 * 1000)
#define FAN_DUR_US            (3ull * 60 * 1000 * 1000)
#define OFF_DUR_US            (27ull * 60 * 1000 * 1000)
#define ADJUST_STEP_US        (15ull * 60 * 1000 * 1000)

static float cool_cycle_duration_us = COOL_DUR_US;

static ac_state internal_state;
static absolute_time_t state_entered;
static absolute_time_t cooling_started;
static absolute_time_t cooling_ended_at;
static bool cooling_phase;
static bool initialized;

static bool is_cooling(ac_state s) {
    return s == AC_COOL_LOW || s == AC_COOL_MED || s == AC_COOL_HIGH;
}

static const char *state_name(ac_state s) {
    switch (s) {
    case AC_OFF:       return "OFF";
    case AC_FAN:       return "FAN";
    case AC_COOL_LOW:  return "COOL_LOW";
    case AC_COOL_MED:  return "COOL_MED";
    case AC_COOL_HIGH: return "COOL_HIGH";
    default:           return "???";
    }
}

static void ti_init(void) {
    initialized = true;
    internal_state = AC_COOL_LOW;
    state_entered = get_absolute_time();
    cooling_started = get_absolute_time();
    cooling_phase = true;
    cooling_ended_at = nil_time;
}

static void apply_transition(ac_state to, absolute_time_t now) {
    if (is_cooling(internal_state) && !is_cooling(to)) {
        cooling_ended_at = now;
    }
    internal_state = to;
    state_entered = now;
    if (to == AC_COOL_HIGH || to == AC_COOL_MED || to == AC_COOL_LOW) {
        cooling_started = now;
        cooling_phase = true;
    } else if (to == AC_FAN || to == AC_OFF) {
        cooling_phase = false;
    }
}

static ac_state ti_evaluate(float temp, float hum) {
    if (!initialized) ti_init();

    absolute_time_t now = get_absolute_time();
    int64_t time_in_state = absolute_time_diff_us(state_entered, now);
    int64_t cooling_elapsed = cooling_phase
        ? absolute_time_diff_us(cooling_started, now)
        : 0;

    ac_state desired = internal_state;
    const char *reason = "no change";

    switch (internal_state) {
    case AC_OFF:
        if (time_in_state >= OFF_DUR_US) {
            desired = AC_COOL_LOW;
            reason = "off duration elapsed";
        }
        break;

    case AC_COOL_HIGH:
    case AC_COOL_MED:
    case AC_COOL_LOW:
        if (cooling_elapsed >= cool_cycle_duration_us) {
            desired = AC_FAN;
            reason = "cool cycle duration expired";
        }
        break;

    case AC_FAN:
        if (time_in_state >= FAN_DUR_US) {
            desired = AC_OFF;
            reason = "fan duration elapsed";
        }
        break;

    default:
        break;
    }

    ac_state actual;
    if (desired != internal_state) {
      printf("%lld minutes in %s, → %s\n", time_in_state/1000/1000/60,
             state_name(internal_state), state_name(desired));
      apply_transition(desired, now);
      actual = desired;
    } else {
        printf("%lld minutes in %s, staying (%s)\n",
               time_in_state/1000/1000/60, state_name(internal_state), reason);
        actual = internal_state;
    }

    return actual;
}

static void ti_adjust_target(float delta) {
    cool_cycle_duration_us += delta * ADJUST_STEP_US;
    if (cool_cycle_duration_us < COOL_DUR_MIN_US) cool_cycle_duration_us = COOL_DUR_MIN_US;
    if (cool_cycle_duration_us > COOL_DUR_MAX_US) cool_cycle_duration_us = COOL_DUR_MAX_US;
}

static float ti_get_cool_duration_min(void) {
    return cool_cycle_duration_us/1000/1000/60;
}

const ac_strategy strategy_time_interval = {
    .name = "time interval",
    .evaluate = ti_evaluate,
    .adjust_target = ti_adjust_target,
    .get_target = ti_get_cool_duration_min,
    .target_unit = "min",
};
