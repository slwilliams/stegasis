#include "fs/fswrapper.h"
#include "fs/stegfs.h"

int wrap_getattr(const char *path, struct stat *stbuf) {
  return SteganographicFileSystem::Instance()->getattr(path, stbuf);
}

int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->readdir(path, buf, filler, offset, fi);
}

int wrap_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->read(path, buf, size, offset, fi);
}

int wrap_open(const char *path, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->open(path, fi);
}

int wrap_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->write(path, buf, size, offset, fi);
}

int wrap_access(const char *path, int mask) {
  return SteganographicFileSystem::Instance()->access(path, mask);
}

int wrap_truncate(const char *path, off_t newsize) {
  return SteganographicFileSystem::Instance()->truncate(path, newsize);
}

int wrap_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->create(path, mode, fi);
}

int wrap_utime(const char *path, struct utimbuf *ubuf) {
  return SteganographicFileSystem::Instance()->utime(path, ubuf);
}

int wrap_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->fsync(path, datasync, fi);
}

struct fuse_operations fs_oper;

void wrap_mount(std::string mountPoint) {
  char **argv = (char **)malloc(3 * sizeof(char*));
  argv[0] = (char *)malloc(sizeof(char)*5);
  argv[1] = (char *)malloc(sizeof(char)*mountPoint.length() + 1);
  for(int i = 0; i < mountPoint.length(); i ++) {
    argv[1][i] = mountPoint.c_str()[i];
  }
  argv[1][mountPoint.length()] = '\0';
  argv[2] = (char *)malloc(sizeof(char)*3);
  argv[2][0] = '-'; 
  argv[2][1] = 'f'; 
  argv[2][2] = '\0';

  fs_oper.getattr  = wrap_getattr;
  fs_oper.readdir  = wrap_readdir;
  fs_oper.open     = wrap_open; 
  fs_oper.read     = wrap_read; 
  fs_oper.write    = wrap_write;
  fs_oper.access   = wrap_access;
  fs_oper.truncate = wrap_truncate;
  fs_oper.create   = wrap_create;
  fs_oper.utime    = wrap_utime;
  fs_oper.fsync    = wrap_fsync;

  fuse_main(3, argv, &fs_oper, NULL);
}
