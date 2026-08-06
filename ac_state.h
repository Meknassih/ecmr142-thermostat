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
typedef void (*ac_adjust_target_fn)(float delta);
typedef float (*ac_get_target_fn)(void);

typedef struct {
    const char *name;
    ac_strategy_fn evaluate;
    ac_adjust_target_fn adjust_target;
    ac_get_target_fn get_target;
    const char *target_unit;
} ac_strategy;

void ac_state_init(void);
void ac_state_set_strategy(const ac_strategy *s);
ac_state ac_state_evaluate(float temp, float hum);
ac_state ac_state_get(void);
bool ac_state_changed(void);
void ac_state_str(ac_state s, char *buf, size_t len);
bool ac_state_adjust_target(float delta);
float ac_state_get_target(void);
const char *ac_state_target_unit(void);

#endif