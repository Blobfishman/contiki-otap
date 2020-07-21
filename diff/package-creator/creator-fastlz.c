#include <stdio.h>
#include <stdlib.h>

#include "../FastLZ/fastlz.h"
#include "../minidiff/bsdiff.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("wrong argument: new_file compressed-files");
    return -1;
  }
  FILE *innew = fopen(argv[1], "r");
  if (!innew) {
    printf("opening input file (new) failed");
    return -1;
  }

  // get file size
  fseek(innew, 0, SEEK_END);
  long fsize_new = ftell(innew);
  fseek(innew, 0, SEEK_SET);

  // read the whole file
  char *in_new_buff = malloc(fsize_new + 1);
  fread(in_new_buff, 1, fsize_new, innew);
  fclose(innew);

  // compress diff
  long compressed_buff_size = fsize_new * 1.05;
  char *compressed_buff = malloc(compressed_buff_size);
  long file_size =
      fastlz_compress_level(2, in_new_buff, fsize_new, compressed_buff);

  free(in_new_buff);
  // write diff file
  FILE *output = fopen(argv[2], "wr");
  if (!output) {
    printf("opening output file failed");
    free(compressed_buff);
    return -1;
  }
  int ret = fwrite(compressed_buff, file_size, 1, output);
  if (!ret) {
    printf("writing to file failed");
    free(compressed_buff);
    return -1;
  }
  fclose(output);
  free(compressed_buff);

  return 0;
}
