#include "ac_strategy_economic.h"
#include "humidex.h"
#include <pico/time.h>
#include <stdio.h>

#define DEBOUNCE_DUR_US       (60ull * 1000 * 1000)
#define COOL_CYCLE_DUR_US     (60ull * 60 * 1000 * 1000)
#define FAN_DUR_US            (3ull * 60 * 1000 * 1000)
#define COMPRESSOR_BACKOFF_US (15ull * 60 * 1000 * 1000)

#define TARGET_HUMIDEX_DEFAULT 29.0f
#define TARGET_HUMIDEX_MIN     20.0f
#define TARGET_HUMIDEX_MAX     40.0f
#define DEADBAND_HIGH         3.0f
#define GENTLE_BAND           1.0f
#define GENTLE_HYST           0.25f

static float target_humidex = TARGET_HUMIDEX_DEFAULT;

static ac_state internal_state;
static absolute_time_t state_entered;
static absolute_time_t cooling_started;
static absolute_time_t cooling_ended_at;
static bool cooling_phase;
static bool backoff_armed;
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

static void economic_init(void) {
    initialized = true;
    internal_state = AC_OFF;
    state_entered = get_absolute_time();
    cooling_phase = false;
    backoff_armed = false;
    cooling_ended_at = nil_time;
}

static void apply_transition(ac_state to, absolute_time_t now) {
    if (is_cooling(internal_state) && !is_cooling(to)) {
        cooling_ended_at = now;
        backoff_armed = true;
    }
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
    float feels_like = humidex_compute(temp, hum);
    float delta = feels_like - target_humidex;
    int64_t time_in_state = absolute_time_diff_us(state_entered, now);
    int64_t cooling_elapsed = cooling_phase
        ? absolute_time_diff_us(cooling_started, now)
        : 0;

    ac_state desired = internal_state;
    const char *reason = "no change";

    switch (internal_state) {
    case AC_OFF:
        if (delta > DEADBAND_HIGH) {
            desired = AC_COOL_HIGH;
            reason = "delta > deadband";
        }
        break;

    case AC_COOL_HIGH:
        if (cooling_elapsed >= COOL_CYCLE_DUR_US) {
            desired = AC_FAN;
            reason = "cool cycle duration expired";
        } else if (delta <= GENTLE_BAND) {
            desired = AC_COOL_LOW;
            reason = "delta within gentle band";
        }
        break;

    case AC_COOL_LOW:
        if (delta > GENTLE_BAND + GENTLE_HYST) {
            desired = AC_COOL_HIGH;
            reason = "delta re-escalated";
        } else if (delta <= 0) {
            desired = AC_FAN;
            reason = "target humidex reached";
        } else if (cooling_elapsed >= COOL_CYCLE_DUR_US) {
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

    if (is_cooling(desired) && !is_cooling(internal_state) && backoff_armed) {
        int64_t since_off = absolute_time_diff_us(cooling_ended_at, now);
        if (since_off < COMPRESSOR_BACKOFF_US) {
            int64_t left_s = (COMPRESSOR_BACKOFF_US - since_off) / 1000000;
            printf("ΔHdx=%+.1f — would enter %s, blocked by compressor backoff %llds\n",
                   delta, state_name(desired), left_s);
            desired = internal_state;
        }
    }

    ac_state actual;
    if (desired != internal_state) {
        if (time_in_state >= DEBOUNCE_DUR_US) {
            printf("ΔHdx=%+.1f — %s, switching %s → %s\n",
                   delta, reason,
                   state_name(internal_state), state_name(desired));
            apply_transition(desired, now);
            actual = desired;
        } else {
            int64_t left_s = (DEBOUNCE_DUR_US - time_in_state) / 1000000;
            printf("ΔHdx=%+.1f — would switch %s → %s (%s), blocked by debounce %llds\n",
                   delta,
                   state_name(internal_state), state_name(desired), reason,
                   left_s);
            actual = internal_state;
        }
    } else {
        printf("ΔHdx=%+.1f — staying %s (%s)\n",
               delta, state_name(internal_state), reason);
        actual = internal_state;
    }

    return actual;
}

static void economic_adjust_target(float delta) {
    target_humidex += delta;
    if (target_humidex < TARGET_HUMIDEX_MIN) target_humidex = TARGET_HUMIDEX_MIN;
    if (target_humidex > TARGET_HUMIDEX_MAX) target_humidex = TARGET_HUMIDEX_MAX;
    if (initialized) {
        state_entered = delayed_by_us(get_absolute_time(), -DEBOUNCE_DUR_US);
    }
}

static float economic_get_target(void) {
    return target_humidex;
}

const ac_strategy strategy_economic = {
    .name = "economic",
    .evaluate = economic_evaluate,
    .adjust_target = economic_adjust_target,
    .get_target = economic_get_target,
    .target_unit = "Hdx",
};
