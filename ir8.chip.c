#include "wokwi-api.h"
#include <stdlib.h>
#include <stdio.h>
typedef struct {
  pin_t out[8], vcc, ir, trig;
  uint32_t mode, mask, prevMode, previous;
  uint64_t start;
  bool running;
  buffer_t fb;
  uint32_t pixels[28][160];
} chip_t;
static void tick(void *data) {
  chip_t *c = data;
  uint32_t mode = attr_read(c->mode);
  if (mode != c->prevMode) {
    c->running = false;
    c->prevMode = mode;
  }
  if (!c->running && pin_read(c->trig)) {
    c->running = true;
    c->start = get_sim_nanos();
  }
  double t = c->running ? (get_sim_nanos() - c->start) / 1e9 : 0;
  int m = 24;
  if (mode == 0)
    m = attr_read(c->mask);
  else if (c->running) {
    if (mode == 1) {
      if (t < 1)
        m = 24;
      else if (t < 1.5)
        m = 12;
      else if (t < 2)
        m = 48;
      else if (t < 3)
        m = 24;
      else if (t < 3.6)
        m = 255;
      else if (t < 3.8)
        m = 3;
      else if (t < 4.25)
        m = 0;
      else if (t < 5.5)
        m = 24;
      else if (t < 6.8)
        m = 0;
      else if (t < 8)
        m = 24;
      else
        m = 255;
    }
    if (mode == 2)
      m = t < 1 ? 24 : 255;
    if (mode == 3)
      m = t < 1 ? 24 : t < 1.7 ? 255 : t < 1.8 ? 254 : t < 2.4 ? 0 : 24;
    if (mode == 4)
      m = t < 1 ? 24 : t < 1.2 ? 3 : 0;
  }
  if (!pin_read(c->vcc) || !pin_read(c->ir))
    m = 0;
  for (int i = 0; i < 8; i++)
    pin_write(c->out[i], !!(m & (128 >> i)));
  for (int y = 0; y < 28; y++)
    for (int x = 0; x < 160; x++) {
      int i = x / 20;
      bool on = m & (128 >> i);
      c->pixels[y][x] = (x % 20 > 2 && x % 20 < 17 && y > 4 && y < 24)
                            ? (on ? 0xffe8eee8 : 0xff393939)
                            : 0xff242b18;
    }
  buffer_write(c->fb, 0, c->pixels, sizeof(c->pixels));
  if (m != c->previous) {
    printf("IR array: 0x%02X at %.2fs\n", m, t);
    c->previous = m;
  }
}
void chip_init() {
  chip_t *c = calloc(1, sizeof(chip_t));
  c->vcc = pin_init("VCC", INPUT);
  pin_init("GND", INPUT);
  c->ir = pin_init("IR", INPUT);
  c->trig = pin_init("TRIG", INPUT_PULLDOWN);
  char name[4];
  for (int i = 0; i < 8; i++) {
    snprintf(name, 4, "D%d", i + 1);
    c->out[i] = pin_init(name, OUTPUT_LOW);
  }
  c->mode = attr_init("mode", 1);
  c->mask = attr_init("mask", 24);
  c->previous = 256;
  uint32_t w, h;
  c->fb = framebuffer_init(&w, &h);
  timer_config_t tc = {.callback = tick, .user_data = c};
  timer_start(timer_init(&tc), 5000, true);
}
