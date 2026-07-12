#include "humidex.h"
#include <math.h>

#define HUMIDEX_REF_PRESSURE    6.11f
#define HUMIDEX_BASELINE        10.0f
#define HUMIDEX_WATER_TRIPLE    273.16f
#define HUMIDEX_LV_OVER_RV      5417.7530f
#define HUMIDEX_C_FACTOR        0.5555f

float humidex_compute(float temp_c, float humidity_pct) {
    if (humidity_pct < 0.0f) humidity_pct = 0.0f;
    if (humidity_pct > 100.0f) humidity_pct = 100.0f;

    float t_kelvin = temp_c + HUMIDEX_WATER_TRIPLE;
    float e_s = HUMIDEX_REF_PRESSURE *
        expf(HUMIDEX_LV_OVER_RV * (1.0f / HUMIDEX_WATER_TRIPLE - 1.0f / t_kelvin));
    float e = e_s * (humidity_pct / 100.0f);

    return temp_c + HUMIDEX_C_FACTOR * (e - HUMIDEX_BASELINE);
}
