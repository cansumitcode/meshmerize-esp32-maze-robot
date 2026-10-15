#include "wokwi-api.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
typedef struct {
  pin_t p, q;
  uint32_t reverse;
  buffer_t fb;
  uint32_t pixels[110][100];
  uint64_t last, window;
  double energy, angle, speed;
  int drive;
  unsigned frames;
} chip_t;
static void accumulate(chip_t *c) {
  uint64_t now = get_sim_nanos();
  c->energy += (double)(now - c->last) * c->drive;
  c->last = now;
}
static void change(void *data, pin_t pin, uint32_t value) {
  chip_t *c = data;
  accumulate(c);
  c->drive = (int)pin_read(c->p) - (int)pin_read(c->q);
}
static void tick(void *data) {
  chip_t *c = data;
  accumulate(c);
  uint64_t now = get_sim_nanos();
  double duty = c->energy / (double)(now - c->window);
  c->energy = 0;
  c->window = now;
  if (attr_read(c->reverse))
    duty = -duty;
  c->speed += (duty - c->speed) * .35;
  c->angle += c->speed * .45;
  for (int y = 0; y < 110; y++)
    for (int x = 0; x < 100; x++) {
      double dx = x - 50, dy = y - 44, r = dx * dx + dy * dy;
      uint32_t color = 0xff222018;
      if (r < 1225)
        color = 0xff777777;
      if (r < 841)
        color = 0xffb2bec2;
      if (r < 841 && fabs(dx * sin(c->angle) - dy * cos(c->angle)) < 2.5)
        color = 0xff35b5ed;
      if (y > 92 && x > 10 && x < 10 + 80 * fabs(c->speed))
        color = c->speed >= 0 ? 0xff80c650 : 0xff638cf2;
      c->pixels[y][x] = color;
    }
  buffer_write(c->fb, 0, c->pixels, sizeof(c->pixels));
  if (++c->frames % 50 == 0)
    printf("N20%s drive=%+.0f%% (normalized, not RPM)\n",
           attr_read(c->reverse) ? " RIGHT" : " LEFT", 100 * duty);
}
void chip_init() {
  chip_t *c = calloc(1, sizeof(chip_t));
  c->p = pin_init("M1", INPUT_PULLDOWN);
  c->q = pin_init("M2", INPUT_PULLDOWN);
  c->reverse = attr_init("reverse", 0);
  uint32_t w, h;
  c->fb = framebuffer_init(&w, &h);
  c->last = c->window = get_sim_nanos();
  pin_watch_config_t cfg = {.edge = BOTH, .pin_change = change, .user_data = c};
  pin_watch(c->p, &cfg);
  pin_watch(c->q, &cfg);
  timer_config_t tc = {.callback = tick, .user_data = c};
  timer_start(timer_init(&tc), 20000, true);
}
