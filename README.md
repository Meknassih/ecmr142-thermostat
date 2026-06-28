# ECMR142 thermostat

Autonomous IR thermostat for (ECMR142)[https://www.boulanger.com/ref/8008803] air conditioners (based on Mistsubishi tech), running on a Raspberry Pi Pico 2 W.

## How It Works

The Pico reads room temperature and humidity from two I2C sensors (AHT20 + BMP280), averages the temperature readings, then evaluates a control strategy that determines the correct cooling state — OFF, FAN, or COOL (low/med/high). When the state changes, the Pico emits the corresponding IR remote control signal via a 38 kHz PWM-driven IR LED, the same way the original remote would.

Control strategies implement a simple `evaluate(temp, hum) → state` interface and are swappable at runtime. The default economic strategy transitions as follows:

| Transition | Condition |
|---|---|
| OFF → COOL_HIGH | Temperature exceeds target (25.5 °C) by > 3.0 °C |
| COOL_HIGH → COOL_LOW | Temperature within 0.5 °C of target |
| COOL_LOW → COOL_HIGH | Temperature rises back above gentle band + hysteresis |
| COOL_LOW → FAN | Target reached (temp ≤ 25.5 °C, humidity ≤ 50%) |
| Any COOL → FAN | Cooling exceeds 30-minute limit |
| FAN → OFF | 2-minute fan dry-out completes |

State changes are debounced (60-second minimum between transitions), the compressor is capped at 30 minutes continuous runtime, and a 2-minute fan-only phase dries the evaporator coil before shutdown.

All limits and thresholds are configurable by editing `#define`s.

## Required Components

| Component | Purpose |
|---|---|
| Raspberry Pi Pico 2 W | Microcontroller |
| AHT20 sensor (I2C addr `0x38`) | Temperature + humidity |
| BMP280 sensor (I2C addr `0x77`) | Temperature (averaged with AHT20) |
| SSD1306 OLED 128×32 (I2C addr `0x3C`) | Status display |
| IR LED + 38 kHz carrier driver circuit | Emits AC commands (on GPIO 16) |
| IR receiver module (optional) | Signal capture / reverse engineering (on GPIO 7) |
| Push button + external LED (optional) | Bootloader trigger + debug (GPIO 5, GPIO 15) |

## Pin Map

| GPIO | Function |
|---|---|
| 5 | External LED |
| 8, 9 | I2C0 (SDA, SCL) for sensors + OLED |
| 7 | IR receiver |
| 15 | Push button (hold to enter flash mode) |
| 16 | IR emitter (PWM, 38 kHz) |

## Build

Requires `arm-none-eabi-gcc`, CMake, and the Pico SDK.

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
make flash   # load via picotool USB
```
