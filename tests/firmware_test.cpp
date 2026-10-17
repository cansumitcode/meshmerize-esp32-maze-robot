// Runs the actual sketch against host stubs, not a second controller implementation.
#include "Arduino.h"
#include <iostream>
#include <stdexcept>
uint32_t fakeMillis = 0;
int inputPins[64] = {}, outputPins[64] = {}, pwmPins[64] = {};
SerialStub Serial;
#include "../sketch.ino"
void require(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
void mask(uint8_t m) {
  for (int i = 0; i < 8; i++)
    inputPins[IR[i]] = !!(m & (128 >> i));
}
void advance(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i += 10) {
    fakeMillis += 10;
    loop();
  }
}
void reset() {
  fakeMillis = 0;
  std::fill(inputPins, inputPins + 64, 0);
  std::fill(outputPins, outputPins + 64, 0);
  std::fill(pwmPins, pwmPins + 64, 0);
  inputPins[START] = inputPins[CAL] = HIGH;
  state = IDLE;
  entered = lastNode = lastReport = lastLoop = 0;
  whiteHigh = true;
  calibrated = lEvidence = rEvidence = leftOldLine = false;
  turnSign = 1;
  turnMin = 160;
  retries = lastError = motorL = motorR = 0;
  Serial.incoming.clear();
  Serial.log.clear();
  mask(24);
  setup();
  advance(40);
}
void run() {
  calibrate();
  startRobot();
  advance(10);
}
int main() {
  int passed = 0;
  auto test = [&](const char *name, auto fn) {
    reset();
    fn();
    ++passed;
    std::cout << "PASS " << name << "\n";
  };
  try {
    test("idle motor inhibition", [] {
      advance(100);
      require(state == IDLE && motorL == 0 && motorR == 0, "motors moved before start");
    });
    test("active-high calibration", [] {
      calibrate();
      require(calibrated && whiteHigh, "calibration");
    });
    test("active-low calibration", [] {
      mask(231);
      calibrate();
      require(calibrated && !whiteHigh && readMask() == 24, "inversion");
    });
    test("invalid calibration inhibits start", [] {
      mask(255);
      startRobot();
      require(state == IDLE && !calibrated && motorL == 0, "invalid start");
    });
    test("centered line and mirrored right motor", [] {
      run();
      require(motorL == 100 && motorR == 100, "center command");
      require(outputPins[M_AIN1] && !outputPins[M_AIN2] && !outputPins[M_BIN1] &&
                  outputPins[M_BIN2],
              "direction pins");
    });
    test("right-offset steering", [] {
      run();
      mask(12);
      advance(30);
      require(motorL > motorR && motorR >= 0, "right steering");
    });
    test("left-offset steering", [] {
      run();
      mask(48);
      advance(30);
      require(motorL < motorR && motorL >= 0, "left steering");
    });
    test("goal requires continuous 1000 ms", [] {
      run();
      mask(255);
      advance(10);
      require(state == CHECK_WHITE, "candidate");
      advance(990);
      require(state == CHECK_WHITE, "early goal");
      advance(10);
      require(state == FINISHED && outputPins[LED] && motorL == 0 && motorR == 0, "goal stop");
    });
    test("seven-white sample cancels goal", [] {
      run();
      mask(255);
      advance(10);
      advance(700);
      mask(254);
      advance(10);
      require(state == JUNCTION && !outputPins[LED], "stale goal timer");
    });
    test("right-first junction choice", [] {
      run();
      mask(255);
      advance(10);
      advance(600);
      mask(24);
      advance(10);
      advance(120);
      require(state == TURN && turnSign == 1, "right priority");
    });
    test("transient line loss recovers without U-turn", [] {
      run();
      mask(0);
      advance(10);
      advance(200);
      mask(24);
      advance(10);
      require(state == FOLLOW, "transient loss");
    });
    test("persistent loss triggers delayed U-turn", [] {
      run();
      mask(0);
      advance(10);
      advance(490);
      require(state == RECOVERY, "early U-turn");
      advance(10);
      require(state == TURN && turnMin == 450, "missing U-turn");
    });
    test("turn requires leaving old line", [] {
      run();
      beginTurn(1, 160);
      advance(300);
      require(state == TURN, "accepted old line");
      mask(0);
      advance(20);
      mask(24);
      advance(10);
      require(state == FOLLOW, "failed reacquisition");
    });
    test("turn timeout retries once then halts", [] {
      run();
      beginTurn(1, 160);
      mask(0);
      advance(2220);
      require(state == TURN && retries == 1, "retry missing");
      advance(2220);
      require(state == HALTED && motorL == 0 && motorR == 0, "failed halt");
    });
    test("serial emergency stop", [] {
      run();
      Serial.incoming.push_back('x');
      advance(10);
      require(state == HALTED && motorL == 0 && motorR == 0 && !outputPins[STBY], "stop");
    });
    test("finished state cannot restart", [] {
      run();
      mask(255);
      advance(1010);
      Serial.incoming.push_back('s');
      mask(24);
      advance(100);
      require(state == FINISHED && motorL == 0 && motorR == 0, "restart after goal");
    });
    std::cout << passed << "/" << passed << " firmware checks passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << "\n";
    return 1;
  }
}
