#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <deque>
constexpr int LOW = 0, HIGH = 1, INPUT = 0, OUTPUT = 1, INPUT_PULLUP = 2;
extern uint32_t fakeMillis;
extern int inputPins[64], outputPins[64], pwmPins[64];
inline uint32_t millis() { return fakeMillis; }
inline void pinMode(int, int) {}
inline int digitalRead(int p) { return inputPins[p]; }
inline void digitalWrite(int p, int v) { outputPins[p] = v; }
template <class T> T constrain(T x, T lo, T hi) { return std::max(lo, std::min(x, hi)); }
inline void ledcAttach(int, int, int) {}
inline void ledcWrite(int p, int v) { pwmPins[p] = v; }
inline void ledcSetup(int, int, int) {}
inline void ledcAttachPin(int, int) {}
struct SerialStub {
  std::deque<char> incoming;
  std::string log;
  void begin(int) {}
  int available() { return incoming.size(); }
  char read() {
    char c = incoming.front();
    incoming.pop_front();
    return c;
  }
  void println(const char *s) {
    log += s;
    log += '\n';
  }
  template <class... A> void printf(const char *fmt, A... args) {
    char b[256];
    std::snprintf(b, sizeof(b), fmt, args...);
    log += b;
  }
};
extern SerialStub Serial;
