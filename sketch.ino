// Meshmerize Wokwi electronics test firmware, reconstructed from project requirements.
// This is NOT the unavailable Meshmerize_FINAL_READY.ino attachment.
#include <Arduino.h>
#include <esp_arduino_version.h>
const uint8_t IR[8] = {36, 39, 34, 35, 32, 33, 25, 26};
const int M_AIN1 = 16, M_AIN2 = 17, PA = 18, M_BIN1 = 19, M_BIN2 = 21, PB = 22, STBY = 23,
          START = 13, CAL = 14, LED = 27;
const uint32_t GOAL_CONFIRM_MS = 1000;
enum State { IDLE, FOLLOW, CHECK_WHITE, JUNCTION, TURN, RECOVERY, HALTED, FINISHED };
State state = IDLE;
uint32_t entered = 0, lastNode = 0, lastReport = 0, lastLoop = 0;
bool whiteHigh = true, calibrated = false, lEvidence = false, rEvidence = false,
     leftOldLine = false;
int turnSign = 1, turnMin = 160, retries = 0, lastError = 0, motorL = 0, motorR = 0;
const char *names[] = {"IDLE",    "FOLLOW",     "CHECKING_WHITE", "JUNCTION",
                       "TURNING", "RECOVERING", "HALTED",         "FINISHED"};
void pwm(int pin, int channel, int value) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, value);
#else
  ledcWrite(channel, value);
#endif
}
void motors(int l, int r) {
  motorL = constrain(l, -165, 165);
  motorR = constrain(r, -165, 165);
  int rr = -motorR; // mirrored right motor
  digitalWrite(M_AIN1, motorL > 0);
  digitalWrite(M_AIN2, motorL < 0);
  digitalWrite(M_BIN1, rr > 0);
  digitalWrite(M_BIN2, rr < 0);
  pwm(PA, 0, abs(motorL));
  pwm(PB, 1, abs(rr));
  digitalWrite(STBY, (motorL != 0 || motorR != 0) ? HIGH : LOW);
}
void enter(State s) {
  state = s;
  entered = millis();
  Serial.printf("STATE %s\n", names[s]);
}
uint8_t rawMask() {
  uint8_t m = 0;
  for (int i = 0; i < 8; i++) {
    int n = 0;
    for (int k = 0; k < 5; k++)
      n += digitalRead(IR[i]);
    if (n >= 3)
      m |= (0x80 >> i);
  }
  return m;
}
uint8_t readMask() {
  uint8_t m = rawMask();
  return whiteHigh ? m : uint8_t(~m);
}
void calibrate() {
  uint8_t m = rawMask();
  bool c1 = m & 0x10, c2 = m & 0x08, e1 = m & 0x80, e2 = m & 0x01;
  if (c1 == c2 && e1 == e2 && c1 != e1) {
    whiteHigh = c1;
    calibrated = true;
    Serial.printf("CAL OK white=%s\n", whiteHigh ? "HIGH" : "LOW");
  } else {
    calibrated = false;
    Serial.println("CAL FAILED: place D4/D5 on white and D1/D8 on black");
  }
}
void beginTurn(int sign, int minimum) {
  turnSign = sign;
  turnMin = minimum;
  leftOldLine = false;
  retries = 0;
  enter(TURN);
  motors(110 * sign, -110 * sign);
}
void startRobot() {
  if (state != IDLE)
    return;
  if (!calibrated) {
    calibrate();
    if (!calibrated)
      return;
  }
  lastError = 0;
  lastNode = millis();
  enter(FOLLOW);
  motors(100, 100);
  Serial.println("START");
}
bool pressed(int pin, int index) {
  static int stable[2] = {HIGH, HIGH}, previous[2] = {HIGH, HIGH};
  static uint32_t changed[2] = {0, 0};
  int v = digitalRead(pin);
  if (v != previous[index]) {
    previous[index] = v;
    changed[index] = millis();
  }
  if (millis() - changed[index] >= 25 && stable[index] != v) {
    stable[index] = v;
    return v == LOW;
  }
  return false;
}
void setup() {
  Serial.begin(115200);
  for (auto p : IR)
    pinMode(p, INPUT);
  for (int p : {M_AIN1, M_AIN2, PA, M_BIN1, M_BIN2, PB, STBY, LED})
    pinMode(p, OUTPUT);
  pinMode(START, INPUT_PULLUP);
  pinMode(CAL, INPUT_PULLUP);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PA, 1000, 8);
  ledcAttach(PB, 1000, 8);
#else
  ledcSetup(0, 1000, 8);
  ledcAttachPin(PA, 0);
  ledcSetup(1, 1000, 8);
  ledcAttachPin(PB, 1);
#endif
  motors(0, 0);
  digitalWrite(LED, LOW);
  Serial.println("MESHMERIZE | CAL then START | serial c=cal s=start x=stop");
  Serial.println("IR demo begins on motor enable. Custom models; no real maze physics.");
}
void loop() {
  if (pressed(CAL, 1) && state == IDLE)
    calibrate();
  if (pressed(START, 0)) {
    if (state == IDLE)
      startRobot();
    else if (state != FINISHED) {
      motors(0, 0);
      enter(HALTED);
      Serial.println("EMERGENCY STOP - reset simulator to restart");
    }
  }
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'c' && state == IDLE)
      calibrate();
    if (c == 's')
      startRobot();
    if (c == 'x' && state != FINISHED) {
      motors(0, 0);
      enter(HALTED);
      Serial.println("EMERGENCY STOP");
    }
  }
  if (millis() - lastLoop < 10)
    return;
  lastLoop = millis();
  uint8_t m = readMask();
  uint32_t age = millis() - entered;
  if (state == IDLE || state == HALTED || state == FINISHED) {
    motors(0, 0);
    return;
  }
  if (state == TURN) {
    motors(110 * turnSign, -110 * turnSign);
    if (!(m & 0x18))
      leftOldLine = true;
    if (leftOldLine && age > uint32_t(turnMin) && (m & 0x18) && m != 0xff) {
      lastNode = millis();
      lastError = 0;
      enter(FOLLOW);
      Serial.println("LINE REACQUIRED");
    } else if (age > 2200) {
      motors(0, 0);
      if (retries++ == 0) {
        Serial.println("TURN TIMEOUT -> one retry");
        entered = millis();
        leftOldLine = false;
      } else {
        enter(HALTED);
        Serial.println("TURN FAILED -> HALT");
      }
    }
    return;
  }
  if (state == CHECK_WHITE) {
    if (m != 0xff) {
      Serial.println("GOAL CANCELLED -> JUNCTION");
      enter(JUNCTION);
      motors(65, 65);
    } else if (age >= GOAL_CONFIRM_MS) {
      motors(0, 0);
      digitalWrite(LED, HIGH);
      enter(FINISHED);
      Serial.println("GOAL CONFIRMED");
    } else
      motors(50, 50);
    return;
  }
  if (state == JUNCTION) {
    lEvidence |= (m & 0xc0) != 0;
    rEvidence |= (m & 3) != 0;
    motors(65, 65);
    if (age >= 120) {
      if (rEvidence) {
        Serial.println("DECISION RIGHT");
        beginTurn(1, 160);
      } else if (m & 0x3c) {
        Serial.println("DECISION STRAIGHT");
        lastNode = millis();
        enter(FOLLOW);
      } else if (lEvidence) {
        Serial.println("DECISION LEFT");
        beginTurn(-1, 160);
      } else {
        Serial.println("DECISION U-TURN");
        beginTurn(1, 450);
      }
    }
    return;
  }
  if (state == RECOVERY) {
    if (m) {
      enter(FOLLOW);
      lastError = 0;
    } else if (age >= 500) {
      Serial.println("DEAD END -> U-TURN");
      beginTurn(1, 450);
    } else
      motors(lastError >= 0 ? 75 : -75, lastError >= 0 ? -75 : 75);
    return;
  }
  if (m == 0xff) {
    lEvidence = rEvidence = true;
    enter(CHECK_WHITE);
    Serial.println("GOAL CANDIDATE: require continuous 8/8 white");
    motors(50, 50);
    return;
  }
  if (!m) {
    enter(RECOVERY);
    return;
  }
  if (millis() - lastNode > 600 && ((m & 0xc0) == 0xc0 || (m & 3) == 3)) {
    lEvidence = (m & 0xc0) != 0;
    rEvidence = (m & 3) != 0;
    enter(JUNCTION);
    return;
  }
  int sum = 0, n = 0;
  for (int i = 0; i < 8; i++)
    if (m & (0x80 >> i)) {
      sum += i * 100 - 350;
      n++;
    }
  int e = n ? sum / n : 0;
  int correction = constrain(int(.30 * e + .65 * (e - lastError)), -90, 90);
  lastError = e;
  motors(constrain(100 + correction, 0, 165), constrain(100 - correction, 0, 165));
  if (millis() - lastReport >= 300) {
    lastReport = millis();
    Serial.printf("IR=%02X L=%d R=%d\n", m, motorL, motorR);
  }
}
