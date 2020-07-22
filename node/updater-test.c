#include "compression.ha"
#include "contiki.h"
#include "sys/log.h"
#include "updater.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO


PROCESS(udp_server_process, "updater test");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data) {
  PROCESS_BEGIN();
  size_t i = 0;
  for (; i < 20; i++) {
    LOG_INFO("updated App");
  }
  struct Updater updater = init_update();
  unsigned char file[1000];
  int old_size = 0;
  int patch = fsd_open(file, 1000);
  int fold = fsd_open(file, 1000);
  int target = fsd_open(file, 1000);

  fastlz_decompress(patch, old_size, target, 1000);

  bspatch(fold, old_size, target, patch);
  patch = fsd_close(patch);
  fold = fsd_close(fold);
  target = fsd_close(target)
  }
  end_update(updater);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/