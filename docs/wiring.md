# Wiring reference

## Eight digital sensor channels

| Sensor | Position | ESP32 GPIO | Wokwi board label |
|---|---|---:|---|
| D1 | Far left | 36 | VP |
| D2 | Left | 39 | VN |
| D3 | Left-center | 34 | D34 |
| D4 | Center-left | 35 | D35 |
| D5 | Center-right | 32 | D32 |
| D6 | Right-center | 33 | D33 |
| D7 | Right | 25 | D25 |
| D8 | Far right | 26 | D26 |

IR VCC and IR enable connect to 3.3V. D4/D5 should see white and D1/D8 should see black during calibration. The fixture defaults to active-high outputs; the firmware also supports active-low calibration. GPIO34–39 are input-only, appropriate for these sensor signals.

## Motor driver and user controls

| Signal | GPIO / connection |
|---|---|
| AIN1 | GPIO16, labeled RX2 |
| AIN2 | GPIO17, labeled TX2 |
| PWMA | GPIO18 |
| BIN1 | GPIO19 |
| BIN2 | GPIO21 |
| PWMB | GPIO22 |
| STBY | GPIO23 |
| AO1 / AO2 | Left motor M1 / M2 |
| BO1 / BO2 | Right motor M1 / M2 |
| Driver VCC | 3.3V |
| Driver VM | Ideal regulated 5V |
| START | GPIO13 to button to ground, internal pull-up |
| CAL | GPIO14 to button to ground, internal pull-up |
| Goal LED | GPIO27 → 220 Ω resistor → LED anode; cathode to ground |

All grounds are common. The right motor is reversed in firmware and marked as mirrored in its display model. PWM is configured for 1 kHz and 8-bit resolution; command values are duty counts, not RPM.

## Simulation-only connections

- IR `TRIG` observes GPIO23 to begin the scripted test stream. Do not add that connection to the physical sensor board.
- The supply switch controls the custom supply's `EN`; the model outputs ideal 5V to VM and VIN. Wokwi internally powers the ESP32, so switching this rail is not a brownout test.
- Serial monitor wiring is part of the simulator interface.
