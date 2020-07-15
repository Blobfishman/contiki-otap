#ifndef UPDATER_H
#define UPDATER_H
#include "contiki.h"
#include "cpu/msp430/flash.c"
#include "config.h"
#include "fs-dummy.h"

struct Updater {
  unsigned short *pos;
};

struct Updater init_update() {
  flash_setup();
  struct Updater u;
  u.pos = (unsigned short *)ROM_START;
  return u;
}

void end_update() {
  flash_done();
  // PROCESS_END();
}

void plain_update(char *data, unsigned size, struct Updater *updater) {
  unsigned short *p_data = (unsigned short *)data;

  size_t i = 0;
  for (; i < size / 2; i++) {
    flash_write(updater->pos, *p_data);
    updater->pos += 1;
    p_data += 1;
  }
}

static unsigned char buf[BUF_SIZE];
static unsigned char tmp[BUF_SIZE];

unsigned int inline uintin(int offset, int fd) {
  unsigned int y;

  fsd_seek(fd, offset, FSD_SEEK_SET);
  fsd_read(fd, buf, 2);

  y = buf[1]; //&0x7F;
  y = y * 256;
  y += buf[0];

  return y;
}

int inline intin(int offset, int fd) {
  int y;
  fsd_seek(fd, offset, FSD_SEEK_SET);
  fsd_read(fd, buf, 2);

  y = buf[1] & 0x7F;
  y = y * 256;
  y += buf[0];

  if (buf[1] & 0x80)
    y = -y;

  return y;
}

unsigned int bspatch(int fold, unsigned int oldsize, int ftarget, int fpatch) {
  unsigned int newsize = 0;
  unsigned int bzctrllen = 0, bzdatalen = 0;
  unsigned int oldpos = 0, newpos = 0, tmp_n = 0;
  unsigned int ctrl[2];
  unsigned int seek;
  unsigned int ctrl_pos, data_pos, extra_pos;
  unsigned int i, j;

  /* Read lengths from header */
  bzctrllen = uintin(0, fpatch);
  bzdatalen = uintin(2, fpatch);
  newsize = uintin(4, fpatch);
  if ((bzctrllen < 0) || (bzdatalen < 0) || (newsize < 0))
    return 0;

  ctrl_pos = 6;
  data_pos = 6 + bzctrllen;
  extra_pos = 6 + bzctrllen + bzdatalen;

  oldpos = 0;
  newpos = 0;
  while (newpos < newsize) {
    /* Read control data */
    for (i = 0; i <= 1; i++) {
      ctrl[i] = uintin(ctrl_pos, fpatch);
      ctrl_pos += 2;
    };
    seek = intin(ctrl_pos, fpatch);
    ctrl_pos += 2;
    /* Sanity-check */
    if (newpos + ctrl[0] > newsize)
      return 0;

    /* Read diff string */
    /* Add old data to diff string */
    fsd_seek(fpatch, data_pos, FSD_SEEK_SET);
    for (i = 0; i < ctrl[0]; i += BUF_SIZE) {

      fsd_read(fpatch, buf, MIN(BUF_SIZE, ctrl[0] - i));
      if ((oldpos + i >= 0) && (oldpos + i < oldsize)) {
        fsd_seek(fold, oldpos + i, FSD_SEEK_SET);
        tmp_n = fsd_read(fold, tmp, MIN(BUF_SIZE, ctrl[0] - i));
        for (j = 0; j < tmp_n; j++)
          buf[j] += tmp[j];
      }
      fsd_write(ftarget, buf, MIN(BUF_SIZE, ctrl[0] - i));
    }
    data_pos += ctrl[0];

    /* Adjust pointers */
    newpos += ctrl[0];
    oldpos += ctrl[0];
    /* Sanity-check */
    if (newpos + ctrl[1] > newsize)
      return 0;

    /* Read extra string */
    for (i = 0; i < ctrl[1]; i += BUF_SIZE) {
      fsd_seek(fpatch, extra_pos + i, FSD_SEEK_SET);
      fsd_read(fpatch, buf, MIN(BUF_SIZE, ctrl[1] - i));
      fsd_write(ftarget, buf, MIN(BUF_SIZE, ctrl[1] - i));
    }
    extra_pos += ctrl[1];

    /* Adjust pointers */
    newpos += ctrl[1];
    oldpos += seek;
  }

  return newsize;
}

#endif