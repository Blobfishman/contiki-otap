#include <stdio.h>
#include <stdlib.h>

#include "../FastLZ/fastlz.h"
#include "../minidiff/bsdiff.h"

long xor_diff(char *old, long size_old, char *new, long size_new, char *patch) {
  if (size_old > size_new) {
    long i = 0;
    patch = malloc(size_new);
    for (; i < size_new; i++) {
      patch[i] = old[i] ^ new[i];
    }
  } else {
    long i = 0;
    patch = malloc(size_new);
    for (; i < size_old; i++) {
      patch[i] = old[i] ^ new[i];
    }
    for (; i < size_new; i++) {
      patch[i] = new[i];
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("wrong argument: old_file new_file patch_file");
    return -1;
  }
  FILE *inold = fopen(argv[1], "r");
  if (!inold) {
    printf("opening input file (old) failed");
    return -1;
  }
  FILE *innew = fopen(argv[2], "r");
  if (!inold) {
    printf("opening input file (new) failed");
    return -1;
  }

  // get file size
  fseek(inold, 0, SEEK_END);
  long fsize_old = ftell(inold);
  fseek(inold, 0, SEEK_SET);
  fseek(innew, 0, SEEK_END);
  long fsize_new = ftell(innew);
  fseek(innew, 0, SEEK_SET);

  // read the whole file
  char *in_old_buff = malloc(fsize_old + 1);
  char *in_new_buff = malloc(fsize_new + 1);
  fread(in_old_buff, 1, fsize_old, inold);
  fread(in_new_buff, 1, fsize_new, innew);
  fclose(inold);
  fclose(innew);

  // create output buffer
  long fsize_patch_max = bsdiff_patchsize_max(fsize_old, fsize_new);
  char *patch_buff = malloc(fsize_patch_max + 1);

  // create diff
  int ret =
      xor_diff(in_old_buff, fsize_old, in_new_buff, fsize_new, patch_buff);
  free(in_old_buff);
  free(in_new_buff);
  if (ret == -1) {
    printf("bsdiff failed\n");
    return -1;
  }

  // compress diff
  long compressed_buff_size = fsize_patch_max * 1.05;
  char *compressed_buff = malloc(compressed_buff_size);
  long file_size =
      fastlz_compress_level(2, patch_buff, fsize_patch_max, compressed_buff);

  // write diff file
  FILE *output = fopen(argv[3], "wr");
  if (!output) {
    printf("opening output file failed");
    free(compressed_buff);
    return -1;
  }
  ret = fwrite(compressed_buff, file_size, 1, output);
  if (!ret) {
    printf("writing to file failed");
    free(compressed_buff);
    return -1;
  }
  fclose(output);
  free(compressed_buff);

  return 0;
}