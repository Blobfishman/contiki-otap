#ifndef FS_DUMMY_H
#define FS_DUMMY_H

#define MAX_FILES 5

typedef unsigned char uint8;

struct Filesystem {
  unsigned char *files[MAX_FILES];
  unsigned char *positions[MAX_FILES];
  unsigned int file_size[MAX_FILES];
  uint8 size;
};

enum SEEK_TYPE { FSD_SEEK_SET, FSD_SEEK_FORWARD, FSD_SEEK_END};

static struct Filesystem gfilesystem = {{0}, {0}, {0}, 0};

// void memcpy(void *dest, void *src, unsigned int n) {
//   // Typecast src and dest addresses to (char *)
//   char *csrc = (char *)src;
//   char *cdest = (char *)dest;

//   // Copy contents of src[] to dest[]
//   unsigned int i = 0;
//   for (; i < n; i++) {
//     cdest[i] = csrc[i];
//   }
// }

static int fsd_read(int fd, void *buf, unsigned int size) {
  if (fd >= gfilesystem.size) {
    return -1;
  }
  memcpy(buf, gfilesystem.positions[fd], size);
  return size;
}

static int fsd_write(int fd, void *buf, unsigned int size) {
  if (fd >= gfilesystem.size) {
    return -1;
  }
  memcpy(gfilesystem.positions[fd], buf, size);
  return size;
}

static int fsd_seek(int fd, int pos, enum SEEK_TYPE type) {
  if (fd >= gfilesystem.size) {
    return -1;
  }
  switch (type) {
  case FSD_SEEK_SET:
    gfilesystem.positions[fd] = gfilesystem.files[fd] + pos;
    break;
  case FSD_SEEK_FORWARD:
    gfilesystem.positions[fd] += pos;
    break;
  case FSD_SEEK_END:
    gfilesystem.positions[fd] = gfilesystem.files[fd] + gfilesystem.file_size[fd];
    pos = gfilesystem.file_size[fd];
  }
  return pos;
}

static int fsd_open(unsigned char *source, unsigned int size) {
  if (gfilesystem.size >= 5) {
    return -1;
  }
  gfilesystem.files[gfilesystem.size] = source;
  gfilesystem.positions[gfilesystem.size] = source;
  gfilesystem.file_size[gfilesystem.size] = size;
  return gfilesystem.size++;
}

static int fsd_close(int fd) {
  if (fd >= gfilesystem.size) {
    return -1;
  }
  gfilesystem.files[gfilesystem.size] = 0;
  gfilesystem.positions[gfilesystem.size] = 0;
  gfilesystem.file_size[gfilesystem.size] = 0;
  return gfilesystem.size--;
}
#endif