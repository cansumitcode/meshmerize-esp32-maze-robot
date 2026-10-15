#include "wokwi-api.h"
#include <stdlib.h>
typedef struct {
  pin_t vm, vcc, stby, a1, a2, pa, b1, b2, pb, ao1, ao2, bo1, bo2;
} chip_t;
static void bridge(pin_t i1, pin_t i2, pin_t pwm, pin_t o1, pin_t o2, bool enabled) {
  if (!enabled) {
    pin_mode(o1, INPUT);
    pin_mode(o2, INPUT);
    return;
  }
  pin_mode(o1, OUTPUT);
  pin_mode(o2, OUTPUT);
  int a = pin_read(i1), b = pin_read(i2);
  // TB6612 short brake for PWM low or both direction inputs high.
  if (!pin_read(pwm) || (a && b)) {
    pin_write(o1, HIGH);
    pin_write(o2, HIGH);
  } else if (!a && !b) {
    pin_mode(o1, INPUT);
    pin_mode(o2, INPUT);
  } else {
    pin_write(o1, a);
    pin_write(o2, b);
  }
}
static void update(void *data, pin_t pin, uint32_t value) {
  chip_t *c = data;
  bool en = pin_read(c->stby) && pin_read(c->vcc) && pin_read(c->vm);
  bridge(c->a1, c->a2, c->pa, c->ao1, c->ao2, en);
  bridge(c->b1, c->b2, c->pb, c->bo1, c->bo2, en);
}
void chip_init() {
  chip_t *c = calloc(1, sizeof(chip_t));
  c->vm = pin_init("VM", INPUT);
  c->vcc = pin_init("VCC", INPUT);
  pin_init("GND", INPUT);
  c->stby = pin_init("STBY", INPUT_PULLDOWN);
  c->a1 = pin_init("AIN1", INPUT_PULLDOWN);
  c->a2 = pin_init("AIN2", INPUT_PULLDOWN);
  c->pa = pin_init("PWMA", INPUT_PULLDOWN);
  c->b1 = pin_init("BIN1", INPUT_PULLDOWN);
  c->b2 = pin_init("BIN2", INPUT_PULLDOWN);
  c->pb = pin_init("PWMB", INPUT_PULLDOWN);
  c->ao1 = pin_init("AO1", INPUT);
  c->ao2 = pin_init("AO2", INPUT);
  c->bo1 = pin_init("BO1", INPUT);
  c->bo2 = pin_init("BO2", INPUT);
  pin_t p[] = {c->vm, c->vcc, c->stby, c->a1, c->a2, c->pa, c->b1, c->b2, c->pb};
  pin_watch_config_t cfg = {.edge = BOTH, .pin_change = update, .user_data = c};
  for (int i = 0; i < 9; i++)
    pin_watch(p[i], &cfg);
  update(c, 0, 0);
}
