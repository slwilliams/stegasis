#include "fs/fswrapper.h"




int wrap_getattr(const char *path, struct stat *stbuf) {
  //SteganographicFileSystem::Instance()->getattr(path, stbuf);
  return 1;
}

int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo) {
 // SteganographicFileSystem::Instance()->readdir(path, buf, filler, offset, fileInfo);
  return 1;
}

int wrap_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
//  SteganographicFileSystem::Instance()->read(path, buf, size, offset, fi);
  return 1;
}

int wrap_open(const char *path, struct fuse_file_info *fi) {
//  SteganographicFileSystem::Instance()->open(path, fi);
  return 1;
}

//struct fuse_operations examplefs_oper;

/*void wrap_mount(std::string mountPoint) {
  char **argv = (char **)malloc(2 * sizeof(char*));
  argv[0] = (char *)malloc(sizeof(char)*9);
  argv[1] = (char *)malloc(sizeof(char)*mountPoint.length() + 1);
  for(int i = 0; i < mountPoint.length(); i ++) {
    argv[1][i] = mountPoint.c_str()[i];
  }
  argv[1][mountPoint.length()] = '\0';

//  examplefs_oper.getattr  = wrap_getattr;
 // examplefs_oper.readdir  = wrap_readdir;
  //examplefs_oper.open   = wrap_open; 
  //examplefs_oper.read   = wrap_read; 

  struct fuse_operations hello_oper = {
  .getattr  = wrap_getattr,
  .readdir  = wrap_readdir,
  .open   = wrap_open,
  .read   = wrap_read,
};

  fuse_main(2, argv, &hello_oper, NULL);
}*/
