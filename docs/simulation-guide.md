# Meshmerize Wokwi component simulation

This project runs real ESP32 Arduino firmware in Wokwi with the documented GPIO wiring.
The original `Meshmerize_FINAL_READY.ino` attachment was not accessible, so `sketch.ino`
is a reconstructed electronics test controller. It does not include the original
branch-memory algorithm and is not a replacement for your competition firmware.

## Included components

| Component | Simulation |
|---|---|
| ESP32 DevKit / WROOM-32 | Native Wokwi ESP32, real compiled Arduino firmware |
| 8-channel digital IR array | Custom `ir8` chip; eight independently driven digital outputs and live display |
| TB6612FNG | Custom `tb6612` chip; standby, both direction pairs, PWM, braking, high-impedance stop |
| Two N20 6V DC motors | Two `n20` chips; read the actual bridge outputs, integrate signed PWM and animate shafts |
| Battery + buck supply | Combined ideal 5V supply model with enable switch; chemistry/capacity unspecified |
| START / emergency stop | Native pushbutton, GPIO13 with internal pull-up |
| CAL | Native pushbutton, GPIO14 with internal pull-up |
| Finish LED + 220 ohm resistor | Native parts, GPIO27 |

All grounds are common. The ESP32's GPIO16/17 pins are labeled RX2/TX2 on this Wokwi board.

## Run in Wokwi

1. Open https://wokwi.com/projects/new/esp32.
2. Replace the contents of `sketch.ino` and `diagram.json` with the matching files here.
3. In the file-tab dropdown, choose **Upload file(s)...** and upload the eight `.chip.c` / `.chip.json` files.
   Uploading does not overwrite the initial sketch or diagram; replace those explicitly.
4. Press the green simulation Play button. Wait for the serial startup message.
5. Click **CALIBRATE**, then **START / STOP**. Alternatively send `c`, then `s`, in the serial monitor.
6. Watch the sensor display, motor shafts, serial events, and red finish LED.
7. To stop an active run, click START again or send `x`. Restart the simulator to reset.

Default mode 1 feeds a repeatable test sequence through the IR chip's physical output pins:
centered line, right/left offsets, broad white junction, right turn with line reacquisition,
lost-line recovery, U-turn, then continuous white goal. The ESP32 does not bypass digitalRead.
Goal confirmation requires a continuous 1000 ms of all eight white inputs.

## IR chip controls

Click the IR chip while running to open its sliders. For repeatable tests, set `mode`
in that component's `attrs` in `diagram.json` before restarting.

| mode | Input sequence |
|---|---|
| 0 | Manual decimal mask slider: 24=center, 12=right offset, 48=left offset, 3=right branch, 192=left branch, 255=all white, 254=seven white, 0=lost |
| 1 | Full demonstration, default |
| 2 | Center then sustained white goal |
| 3 | Eight white for 700 ms, then seven white; must cancel goal candidate |
| 4 | Right branch followed by no line; turn timeout, one retry, then halt |

The gray TRIG wire from GPIO23/STBY is a simulation-only test-fixture connection.
It begins the scripted input stream when the firmware first enables the motors.
It is not a wire to add to the physical IR sensor board.

## Wiring

- D1–D8: GPIO36, 39, 34, 35, 32, 33, 25, 26 (robot left to right).
- IR VCC and IR enable: 3.3V; digital white is HIGH in the default model.
- AIN1/2: GPIO16/17; PWMA: GPIO18; AO1/2: left motor.
- BIN1/2: GPIO19/21; PWMB: GPIO22; BO1/2: right motor.
- STBY: GPIO23; driver logic VCC: 3.3V; driver VM: ideal 5V.
- Right motor mounting is reversed, matching the project's stated configuration.

## Scope and limitations

This is an electronics/control-logic simulation, not a closed-loop moving robot in a maze.
IR values are scripted or manual, not computed from motor position. Turning requires
sensor loss and reacquisition with a timeout, but cannot prove a physical turn angle.
The lightweight DFS branch-memory requirement is not implemented in this reconstructed test sketch.

N20 gearing/RPM was unspecified. Motor displays report normalized signed PWM, not calibrated RPM.
The simple smoothing/shaft animation does not model torque, friction, back-EMF, current, load or stall.
The bridge model uses digital logic; it does not model a 5V power waveform or electrical stress.
The ideal supply does not model a particular battery, buck efficiency, regulator ripple, or ESP32 brownout.
Switching the supply disables the simulated motor rail; Wokwi's ESP32 remains internally powered.

Reference: https://docs.wokwi.com/chips-api/getting-started
Native supported hardware: https://docs.wokwi.com/getting-started/supported-hardware
