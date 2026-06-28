#ifndef AC_STATE_H
#define AC_STATE_H
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    AC_OFF,
    AC_FAN,
    AC_COOL_LOW,
    AC_COOL_MED,
    AC_COOL_HIGH,
} ac_state;

typedef ac_state (*ac_strategy_fn)(float temp, float hum);

typedef struct {
    const char *name;
    ac_strategy_fn evaluate;
} ac_strategy;

void ac_state_init(void);
void ac_state_set_strategy(const ac_strategy *s);
ac_state ac_state_evaluate(float temp, float hum);
ac_state ac_state_get(void);
bool ac_state_changed(void);
void ac_state_str(ac_state s, char *buf, size_t len);

#endif