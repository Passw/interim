#include <stdio.h>
#include "stream.h"
#include "alloc.h"
#include "writer.h"

static Cell* fs_list;
static int num_fs;
static int stream_id;

typedef Cell* (*funcptr2)(Cell* a1, Cell* a2);

// TODO: include fs_list in gc mark

Cell* get_fs_list() {
  return fs_list;
}

Cell* fs_open(Cell* path) {
  if (!path || path->tag!=TAG_STR) {
    printf("[open] error: string required.");
    return alloc_nil();
  }
  
  Cell* fsl = fs_list;
  Cell* fs_cell;
  while ((fs_cell = car(fsl))) {
    Filesystem* fs = (Filesystem*)fs_cell->next;
    //printf("compare %s : %s : %p\n",fs->mount_point->addr, path->addr, strstr(fs->mount_point->addr, path->addr));
    if (path->addr && strstr(path->addr, fs->mount_point->addr) == path->addr) {
      printf("[open] found matching fs: %s for path: %s\n", fs->mount_point->addr, path->addr);
      Stream* s = malloc(sizeof(Stream));
      s->fs = fs;
      s->path = path;
      s->id = stream_id;

      // TODO: integrate in GC

      Cell* stream_cell = alloc_int(stream_id);
      stream_cell->tag = TAG_STREAM;
      stream_cell->addr = s;
      stream_id++;

      // open the filesystem
      //printf("[open] open_fn: %p\n", s->fs->open_fn);
      if (s->fs->open_fn && s->fs->open_fn->next) {
        Cell* open_fn = s->fs->open_fn;
        ((funcptr2)open_fn->next)(path, NULL);
      }

      return stream_cell;
    }
    fsl = cdr(fsl);
  }
  return alloc_nil();
}

Cell* fs_mmap(Cell* path) {
  if (!path || path->tag!=TAG_STR) {
    printf("[mmap] error: string required.");
    return alloc_nil();
  }
  
  Cell* fsl = fs_list;
  Cell* fs_cell;
  while ((fs_cell = car(fsl))) {
    Filesystem* fs = (Filesystem*)fs_cell->next;
    if (path->addr && strstr(path->addr, fs->mount_point->addr) == path->addr) {
      printf("[mmap] found matching fs: %s for path: %s\n", fs->mount_point->addr, path->addr);

      if (fs->mmap_fn && fs->mmap_fn->next) {
        Cell* mmap_fn = fs->mmap_fn;
        return (Cell*)((funcptr2)mmap_fn->next)(path, NULL);
      } else {
        printf("[mmap] error: fs has no mmap implementation.");
        return alloc_nil();
      }
    }
    fsl = cdr(fsl);
  }
  return alloc_nil();
}

Cell* fs_mount(Cell* path, Cell* handlers) {
  if (!path || path->tag!=TAG_STR) {
    printf("[mount] error: string required.");
    return alloc_nil();
  }
  
  Filesystem* fs = malloc(sizeof(Filesystem));
  fs->open_fn = car(handlers);
  handlers = cdr(handlers);
  fs->read_fn = car(handlers);
  handlers = cdr(handlers);
  fs->write_fn = car(handlers);
  handlers = cdr(handlers);
  fs->delete_fn = car(handlers);
  handlers = cdr(handlers);
  fs->mmap_fn = car(handlers);
  handlers = cdr(handlers);
  fs->mount_point = path;
  fs->close_fn = NULL;

  Cell* fs_cell = alloc_int(num_fs++);
  fs_cell->next = fs;
  fs_cell->tag = TAG_FS;
  
  printf("[fs] mounted: %s\n",path->addr);
  fs_list = alloc_cons(fs_cell, fs_list);

  return fs_cell;
}

Cell* wrap_in_lambda(void* cfunc) {
  Cell* lbd = alloc_lambda(alloc_nil());
  lbd->next = cfunc;
  return lbd;
}

// TODO: pass stream (context) to handlers

Cell* stream_read(Cell* stream) {
  if (!stream || stream->tag!=TAG_STREAM) {
    printf("[fs] error: non-stream passed to recv\r\n");
    return alloc_nil();
  }
  Stream* s = (Stream*)stream->addr;
  Cell* read_fn = s->fs->read_fn;
  //char debug_buf[256];
  //lisp_write(read_fn, debug_buf, 256);
  //printf("[stream_read] fn: %s ptr: %p\n",debug_buf,read_fn->next);
  return (Cell*)((funcptr2)read_fn->next)(stream,NULL);
}

Cell* stream_write(Cell* stream, Cell* arg) {
  if (!stream || stream->tag!=TAG_STREAM) {
    printf("[fs] error: non-stream passed to send\r\n");
    return alloc_nil();
  }
  Stream* s = (Stream*)stream->addr;
  Cell* write_fn = s->fs->write_fn;
  //char debug_buf[256];
  //lisp_write(arg, debug_buf, 256);
  //printf("[stream_write] fn: %s ptr: %p\n",debug_buf,write_fn->next);
  //printf("[stream_write] arg: %s\n",debug_buf);
  return (Cell*)((funcptr2)write_fn->next)(stream,arg);
}

void fs_mount_builtin(char* path, void* open_handler, void* read_handler, void* write_handler, void* delete_handler, void* mmap_handler) {
  Cell* handlers = alloc_list((Cell*[]){
        wrap_in_lambda(open_handler),
        wrap_in_lambda(read_handler),
        wrap_in_lambda(write_handler),
        wrap_in_lambda(delete_handler),
        wrap_in_lambda(mmap_handler),
        },5);
  fs_mount(alloc_string_copy(path), handlers);
}

Cell* consolefs_open() {
  // TODO: set console to raw mode?
  return alloc_int(1);
}

Cell* consolefs_read() {
  int c = fgetc(stdin);
  return alloc_int(c);
}

Cell* consolefs_write(Cell* a1,Cell* arg) {
  fputc(arg->value, stdout);
  return arg;
}

Cell* filesystems_init() {
  fs_list = alloc_nil();
  fs_mount_builtin("/console", consolefs_open, consolefs_read, consolefs_write, 0, 0);
}