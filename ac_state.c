#include "ac_state.h"
#include <pico/time.h>
#include <stdio.h>
#include <string.h>

static ac_state current_state;
static const ac_strategy *active_strategy;
static absolute_time_t state_entered_at;
static bool changed;

void ac_state_init(void) {
    current_state = AC_OFF;
    state_entered_at = get_absolute_time();
    changed = false;
    active_strategy = NULL;
}

void ac_state_set_strategy(const ac_strategy *s) {
    active_strategy = s;
}

ac_state ac_state_evaluate(float temp, float hum) {
    if (!active_strategy) return current_state;

    ac_state new_state = active_strategy->evaluate(temp, hum);

    if (new_state != current_state) {
        current_state = new_state;
        state_entered_at = get_absolute_time();
        changed = true;
    } else {
        changed = false;
    }

    return current_state;
}

ac_state ac_state_get(void) {
    return current_state;
}

bool ac_state_changed(void) {
    return changed;
}

void ac_state_str(ac_state s, char *buf, size_t len) {
    switch (s) {
    case AC_OFF:
        strncpy(buf, "Off", len);
        break;
    case AC_FAN:
        strncpy(buf, "Fan", len);
        break;
    case AC_COOL_LOW:
        strncpy(buf, "Cool low", len);
        break;
    case AC_COOL_MED:
        strncpy(buf, "Cool med", len);
        break;
    case AC_COOL_HIGH:
        strncpy(buf, "Cool high", len);
        break;
    }
    buf[len - 1] = '\0';
}