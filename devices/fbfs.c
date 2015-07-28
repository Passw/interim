#define WIDTH 1920
#define HEIGHT 1080
#define BPP 2
#define DEPTH 16

#include "stream.h"

static uint32_t* _fb;

Cell* fbfs_open() {
  return alloc_int(1);
}

Cell* fbfs_read() {
  return alloc_int(0);
}

Cell* fbfs_delete() {
  return alloc_int(0);
}

Cell* fbfs_write(Cell* arg) {
  return NULL;
}

Cell* fbfs_mmap(Cell* arg) {
  long sz = WIDTH*HEIGHT*BPP;
  printf("[fbfs_mmap] addr: %p\n",_fb);

  if (_fb>0) {
    Cell* buffer_cell = alloc_int(0);
    buffer_cell->addr = _fb;
    buffer_cell->size = sz;
    buffer_cell->tag = TAG_BYTES;
    printf("[fbfs_mmap] buffer_cell->addr: %p\n",buffer_cell->addr);
  
    return buffer_cell;
  } else {
    return alloc_nil();
  }
}

void mount_fbfs(uint32_t* fb) {
  _fb = fb;
  fs_mount_builtin("/framebuffer", fbfs_open, fbfs_read, fbfs_write, fbfs_delete, fbfs_mmap);
}
