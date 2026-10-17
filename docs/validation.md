# Validation and evidence

## Wokwi run — October 10, 2026

The ESP32 sketch and custom C chips compiled in Wokwi. The saved project exercised calibration, line following with both steering signs, temporary-white cancellation, right-turn reacquisition, dead-end recovery, U-turn reacquisition and final goal stop.

- [Full demo serial output](evidence/demo-serial-log.txt)
- [Physical simulator-button emergency-stop output](evidence/emergency-stop-log.txt)
- [Motor console excerpt](evidence/motor-log.txt)
- [Circuit at goal](assets/wokwi-demo.png)

The motor excerpt was captured around a short start/stop run and reports zero-drive samples. It is not evidence of calibrated motor speed or performance. Serial logs report commanded motor values; the LED screenshot supports the finish-state observation.

## Host regression tests — October 15, 2026

`tests/firmware_test.cpp` includes the actual `sketch.ino` and replaces Arduino services with small deterministic stubs. Sixteen scenarios cover idle inhibition, both calibration polarities, invalid calibration, centered/offset steering, mirrored direction pins, the 1,000 ms goal threshold, seven-white cancellation, right-first selection, transient/persistent line loss, turn reacquisition, turn timeout/retry, emergency stop and terminal finish behavior.

[Recorded local result](evidence/host-tests.txt)

`tests/validate_project.py` checks JSON definitions, pin references, exact documented GPIO mappings, required power/control connections, and absence of duplicate component identifiers.

GitHub Actions runs these checks on pushes and pull requests. The workflow is a host test, not an ESP32 cross-compile or a Wokwi CI run; no paid Wokwi CI token is needed.

## Not yet demonstrated

- A robot navigating a physical maze or a motor-feedback-driven virtual maze.
- Physical line-tracking accuracy, speed, completion rate, battery endurance or exact turn angles.
- General junction classification or DFS branch memory.
- Interactive Wokwi runs for every selectable input mode. The saved full-demo and stop evidence should not be generalized to an exhaustive hardware test suite.
