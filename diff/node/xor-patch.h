#ifndef XOR_PATCH_H
#define XOR_PATCH_H

#include "contiki.h"
#include "sys/log.h"
#include <stdio.h>

int xor_patch(unsigned char *old, int size_old,unsigned char *new_buf, int size_new,unsigned char *patch) {
  if (size_old > size_new) {
    int i = 0;
    for (; i < size_new; i++) {
      patch[i] = old[i] ^ new_buf[i];
    }
    for (; i < size_new; i++) {
      patch[i] = 0;
    }
  } else {
    int i = 0;
    for (; i < size_old; i++) {
      patch[i] = old[i] ^ new_buf[i];
    }
    for (; i < size_new; i++) {
      patch[i] = new_buf[i];
    }
  }
  return size_new;
}
#endif