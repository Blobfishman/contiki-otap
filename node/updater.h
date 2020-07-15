#define ROM_START 0x4000

struct Updater {
  unsigned short *pos;
};

struct Updater init_update();

void end_update(struct Updater update));

void plain_update(char *data, unsigned size, struct Updater updater);

void bsdiff_update(char *data, unsigned size, struct Updater updater);