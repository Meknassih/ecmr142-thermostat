#include "ac_strategy_economic.h"
#include <pico/time.h>
#include <stdio.h>

#define DEBOUNCE_DUR_US       (60ull * 1000 * 1000)
#define MAX_COOL_DUR_US       (30ull * 60 * 1000 * 1000)
#define FAN_DUR_US            (2ull * 60 * 1000 * 1000)

#define TARGET_TEMP           25.5f
#define TARGET_HUM            52.0f
#define DEADBAND_HIGH          2.0f
#define GENTLE_BAND            0.5f
#define GENTLE_HYST            0.3f

static ac_state internal_state;
static absolute_time_t state_entered;
static absolute_time_t cooling_started;
static bool cooling_phase;
static bool initialized;

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

static void economic_init(void) {
    initialized = true;
    internal_state = AC_OFF;
    state_entered = get_absolute_time();
    cooling_phase = false;
}

static void apply_transition(ac_state to, absolute_time_t now) {
    internal_state = to;
    state_entered = now;
    if (to == AC_COOL_HIGH) {
        cooling_started = now;
        cooling_phase = true;
    } else if (to == AC_FAN || to == AC_OFF) {
        cooling_phase = false;
    }
}

static ac_state economic_evaluate(float temp, float hum) {
    if (!initialized) economic_init();

    absolute_time_t now = get_absolute_time();
    float delta_t = temp - TARGET_TEMP;
    float delta_h = hum - TARGET_HUM;
    int64_t time_in_state = absolute_time_diff_us(state_entered, now);
    int64_t cooling_elapsed = cooling_phase
        ? absolute_time_diff_us(cooling_started, now)
        : 0;

    ac_state desired = internal_state;
    const char *reason = "no change";

    switch (internal_state) {
    case AC_OFF:
        if (delta_t > DEADBAND_HIGH) {
            desired = AC_COOL_HIGH;
            reason = "delta_t > deadband";
        }
        break;

    case AC_COOL_HIGH:
        if (cooling_elapsed >= MAX_COOL_DUR_US) {
            desired = AC_FAN;
            reason = "max cooling time expired";
        } else if (delta_t <= GENTLE_BAND) {
            desired = AC_COOL_LOW;
            reason = "delta_t within gentle band";
        }
        break;

    case AC_COOL_LOW:
        if (delta_t > GENTLE_BAND + GENTLE_HYST) {
            desired = AC_COOL_HIGH;
            reason = "delta_t re-escalated";
        } else if (delta_t <= 0 && delta_h <= 0) {
            desired = AC_FAN;
            reason = "target temp+hum reached";
        } else if (cooling_elapsed >= MAX_COOL_DUR_US) {
            desired = AC_FAN;
            reason = "max cooling time expired";
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
        if (time_in_state >= DEBOUNCE_DUR_US) {
            printf("ΔT=%+.1f°C ΔH=%+.1f%% — %s, switching %s → %s\n",
                   delta_t, delta_h, reason,
                   state_name(internal_state), state_name(desired));
            apply_transition(desired, now);
            actual = desired;
        } else {
            int64_t left_s = (DEBOUNCE_DUR_US - time_in_state) / 1000000;
            printf("ΔT=%+.1f°C ΔH=%+.1f%% — would switch %s → %s (%s), blocked by debounce %llds\n",
                   delta_t, delta_h,
                   state_name(internal_state), state_name(desired), reason,
                   left_s);
            actual = internal_state;
        }
    } else {
        printf("ΔT=%+.1f°C ΔH=%+.1f%% — staying %s (%s)\n",
               delta_t, delta_h, state_name(internal_state), reason);
        actual = internal_state;
    }

    return actual;
}

const ac_strategy strategy_economic = {
    .name = "economic",
    .evaluate = economic_evaluate,
};