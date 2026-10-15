#include "wokwi-api.h"
#include <stdlib.h>
typedef struct {
  pin_t en, out;
  uint32_t enabled;
} chip_t;
static void tick(void *data) {
  chip_t *c = data;
  pin_dac_write(c->out, (attr_read(c->enabled) && pin_read(c->en)) ? 5.0 : 0.0);
}
void chip_init() {
  chip_t *c = calloc(1, sizeof(chip_t));
  c->en = pin_init("EN", INPUT_PULLDOWN);
  pin_init("GND", INPUT);
  c->out = pin_init("OUT5V", ANALOG);
  c->enabled = attr_init("enabled", 1);
  timer_config_t t = {.callback = tick, .user_data = c};
  timer_start(timer_init(&t), 1000, true);
}
