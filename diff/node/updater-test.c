#include "contiki.h"
#include "sys/log.h"
#include <stdio.h>

// #include "compression.h"
// #include "updater.h"
// test files
#include "../package-creator/test-files/patch-short.h"
#include "../package-creator/test-files/test.h"
#include "../package-creator/test-files/test-short-compressed.h"
#include "bspatch.h"
#include "fastlz.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define BUFFER_SIZE 1500
PROCESS(udp_server_process, "updater test");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data) {
  PROCESS_BEGIN();
  unsigned char fold_buf[BUFFER_SIZE];
  unsigned char buffer[BUFFER_SIZE];
  LOG_INFO("old:\n");
  LOG_INFO((char *)test_files_test);
  LOG_INFO("\n");
  LOG_INFO("start decompressing");
  fastlz2_decompress(test_files_test_short_compressed, test_files_test_short_compressed_len, buffer, BUFFER_SIZE);
  LOG_INFO("finished decompressing");
  // fastlz2_decompress(test_files_patch_short, test_files_patch_short_len, buffer, BUFFER_SIZE);
  LOG_INFO("start bsdiff");
  bspatch(test_files_test, test_files_test_len, buffer, BUFFER_SIZE, fold_buf, BUFFER_SIZE);
  LOG_INFO("finished bsdiff");
  LOG_INFO("buffer:\n");
  LOG_INFO((char *)BUFFER_SIZE);
  LOG_INFO("\n");
  LOG_INFO("new:\n");
  // LOG_INFO((char *)fold_buf);
  LOG_INFO("\n");
  //   end_update(updater);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
