#include "updater.h"

struct Updater init_update() {
  flash_setup();
  struct Updater u;
  u.pos = ROM_START;
  return u;
}

void end_update() {
  flash_done();
  PROCESS_END();
}

void plain_update(char *data, unsigned size, struct Updater *updater) {
  unsigned short *p_data = (unsigned short *)data;

  for (int i = 0; i < size / 2; i++) {
    flash_write(updater->pos, *p_data);
    updater->pos += 1;
    p_data += 1;
  }
}

void bsdiff_update(char *data, unsigned size, struct Updater updater) {}