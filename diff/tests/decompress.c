#include <stdio.h>
#include <stdlib.h>
#include "../node/fastlz.h"
#include "../node/bspatch.h"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("wrong argument: old_file patch output");
    return -1;
  }
  FILE *inold = fopen(argv[1], "r");
  if(!inold) {
    printf("opening input file (old) failed");
    return -1;
  }
  FILE *patch = fopen(argv[2], "r");
  if(!inold) {
    printf("opening input file (new) failed");
    return -1;
  }
  printf("parameter: %s %s %s\n", argv[1], argv[2], argv[3]);

  // get file size
  fseek(inold, 0, SEEK_END);
  long fsize_old = ftell(inold);
  fseek(inold, 0, SEEK_SET);
  fseek(patch, 0, SEEK_END);
  long fsize_patch = ftell(patch);
  fseek(patch, 0, SEEK_SET);

  // read the whole file
  char *in_old_buff = malloc(fsize_old + 1);
  char *patch_buff = malloc(fsize_patch + 1);
  fread(in_old_buff, 1, fsize_old, inold);
  fread(patch_buff, 1, fsize_patch, inold);
  fclose(inold);
  fclose(patch);

  printf("old-file(%d): ", fsize_old);
  printf(in_old_buff);
  printf("\n");

  printf("patch-file(%d): ", fsize_patch);
  fwrite(patch_buff, sizeof(char), fsize_patch, stdout);
  printf("\n");

  // decompress
  char *patch_decom = malloc(fsize_patch * 4);
  long patch_size = fastlz2_decompress(patch_buff, fsize_patch, patch_decom, fsize_patch * 4);
  printf("decompressed(%d): ", patch_size);
  fwrite(patch_decom, sizeof(char), patch_size, stdout);
  printf("\n");
  // diff
  int output_size = bspatch_newsize(patch_decom, patch_size);
  char* output_buff = malloc(output_size);
  int ret = bspatch(in_old_buff, fsize_old, patch_decom, patch_size, output_buff, output_size);
  printf("bsdiffed(%d): ", ret);
  fwrite(output_buff, sizeof(char), output_size, stdout);
  printf("\n");

  free(patch_decom);
  free(in_old_buff);
  free(patch_buff);
  if (patch_size == -1) {
    printf("decompression failed\n");
    return -1;
  }
  if (ret == -1) {
    printf("bspatch failed\n");
    return -1;
  }

  // write diff file
  FILE *output = fopen(argv[3], "wr");
  if (!output) {
    printf("opening output file failed");
    free(output_buff);
    return -1;
  }
  ret = fwrite(output_buff, output_size, 1, output);
  if (!ret) {
    printf("writing to file failed");
    free(output_buff);
    return -1;
  }
  fclose(output);
  free(output_buff);

  return 0;
}
