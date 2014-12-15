#ifndef wrap_h
#define wrap_h

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#ifdef __cplusplus
  extern "C" {
#endif

int wrap_getattr(const char *path, struct stat *stbuf);
int wrap_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileinfo);
int wrap_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int wrap_open(const char *path, struct fuse_file_info *fi);
int wrap_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int wrap_access(const char *path, int mask);
int wrap_truncate(const char *path, off_t newsize);
int wrap_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int wrap_utime(const char *path, struct utimbuf *ubuf);
int wrap_unlink(const char *path);
int wrap_flush(const char *path, struct fuse_file_info *fi);
int wrap_mkdir(const char *path, mode_t mode);

#ifdef __cplusplus
}
#endif

void wrap_mount(std::string mountPoint);

#endif
