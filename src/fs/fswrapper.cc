#include "fs/fswrapper.h"
#include "fs/stegfs.h"

using namespace std;

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

int wrap_unlink(const char *path) {
  return SteganographicFileSystem::Instance()->unlink(path);
}

int wrap_flush(const char *path, struct fuse_file_info *fi) {
  return SteganographicFileSystem::Instance()->flush(path, fi);
}

int wrap_mkdir(const char *path, mode_t mode) {
  return SteganographicFileSystem::Instance()->mkdir(path, mode);
}

// This cannot be stack allocated...
struct fuse_operations fs_oper;

void wrap_mount(string mountPoint) {
  // Create the directory if it doesn't exist
  string mkdirCommand = "mkdir " + mountPoint + " 2>&1";
  FILE *pipe = popen(mkdirCommand.c_str(), "r");
  pclose(pipe);

  char **argv = (char **)malloc(6 * sizeof(char*));
  argv[0] = (char *)malloc(sizeof(char)*6);
  argv[1] = (char *)malloc(sizeof(char)*mountPoint.length() + 1);
  for(int i = 0; i < mountPoint.length(); i ++) {
    argv[1][i] = mountPoint.c_str()[i];
  }
  argv[1][mountPoint.length()] = '\0';

  argv[2] = (char *)malloc(sizeof(char)*3);
  argv[2][0] = '-'; 
  argv[2][1] = 'f'; 
  argv[2][2] = '\0'; 

  string writes = "-obig_writes";
  argv[3] = (char *)malloc(sizeof(char)*writes.length() + 1);
  for(int i = 0; i < writes.length(); i ++) {
    argv[3][i] = writes.c_str()[i];
  }
  argv[3][writes.length()] = '\0';

  argv[4] = (char *)malloc(sizeof(char)*3);
  argv[4][0] = '-'; 
  argv[4][1] = 'o'; 
  argv[4][2] = '\0'; 

  string r = "max_write=345600";
  argv[5] = (char *)malloc(sizeof(char)*r.length() + 1);
  for(int i = 0; i < r.length(); i ++) {
    argv[5][i] = r.c_str()[i];
  }
  argv[5][r.length()] = '\0';

  fs_oper.getattr  = wrap_getattr;
  fs_oper.readdir  = wrap_readdir;
  fs_oper.open     = wrap_open; 
  fs_oper.read     = wrap_read; 
  fs_oper.write    = wrap_write;
  fs_oper.access   = wrap_access;
  fs_oper.truncate = wrap_truncate;
  fs_oper.create   = wrap_create;
  fs_oper.utime    = wrap_utime;
  fs_oper.unlink   = wrap_unlink;
  fs_oper.flush    = wrap_flush;
  fs_oper.mkdir    = wrap_mkdir;

  fuse_main(6, argv, &fs_oper, NULL);
}
