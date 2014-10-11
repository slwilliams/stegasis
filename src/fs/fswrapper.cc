#include "fs/fswrapper.h"
#include "fs/stegfs.h"

int wrap_getattr(const char *path, struct stat *stbuf) {
  return SteganographicFileSystem::Instance()->getattr(path, stbuf);
}

int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo) {
  return SteganographicFileSystem::Instance()->readdir(path, buf, filler, offset, fileInfo);
}

int wrap_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->read(path, buf, size, offset, fi);
}

int wrap_open(const char *path, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->open(path, fi);
}

struct fuse_operations fs_oper;

void wrap_mount(std::string mountPoint) {
  char **argv = (char **)malloc(2 * sizeof(char*));
  argv[0] = (char *)malloc(sizeof(char)*9);
  argv[1] = (char *)malloc(sizeof(char)*mountPoint.length() + 1);
  for(int i = 0; i < mountPoint.length(); i ++) {
    argv[1][i] = mountPoint.c_str()[i];
  }
  argv[1][mountPoint.length()] = '\0';

  fs_oper.getattr  = wrap_getattr;
  fs_oper.readdir  = wrap_readdir;
  fs_oper.open   = wrap_open; 
  fs_oper.read   = wrap_read; 


  fuse_main(2, argv, &fs_oper, NULL);
}
