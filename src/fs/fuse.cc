#define FUSE_USE_VERSION 26

#include <fuse.h>                                                                      
#include <errno.h> 
#include <fcntl.h>  
#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"

using namespace std;

static const char *hello_str = "Hello World1234!\n";
static const char *hello_path = "/hello";

struct fuse_operations fuse_oper;

class SteganographicFileSystem {
  private:
    VideoDecoder *decoder;
    SteganographicAlgorithm *alg;
  public:
    SteganographicFileSystem(VideoDecoder *decoder, SteganographicAlgorithm *alg):
      decoder(decoder), alg(alg) {
    };
   
    static int getattr(const char *path, struct stat *stbuf)
    {
      int res = 0;

      memset(stbuf, 0, sizeof(struct stat));
      if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
      } else if (strcmp(path, hello_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(hello_str);
      } else {
        res = -ENOENT;
      }

      return res;
    }

    static int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
       off_t offset, struct fuse_file_info *fi)
    {
      if (strcmp(path, "/") != 0)
        return -ENOENT;

      filler(buf, ".", NULL, 0);
      filler(buf, "..", NULL, 0);
      filler(buf, hello_path + 1, NULL, 0);
      return 0;
    }

    static int open(const char *path, struct fuse_file_info *fi)
    {
      if (strcmp(path, hello_path) != 0)
        return -ENOENT;

      if ((fi->flags & 3) != O_RDONLY)
        return -EACCES;

      return 0;
    }

    static int read(const char *path, char *buf, size_t size, off_t offset,
       struct fuse_file_info *fi)
    {
      size_t len;
      if(strcmp(path, hello_path) != 0)
        return -ENOENT;

      len = strlen(hello_str);
      if (offset < len) {
        if (offset + size > len)
          size = len - offset;
        memcpy(buf, hello_str + offset, size);
      } else {
        size = 0;
      }

      return size;
    }


    void mount(string mountPoint) {
      char **argv = (char **)malloc(2 * sizeof(char*));
      argv[0] = (char *)malloc(sizeof(char)*9);
      argv[1] = (char *)malloc(sizeof(char)*mountPoint.length() + 1);
      for(int i = 0; i < mountPoint.length(); i ++) {
        argv[1][i] = mountPoint.c_str()[i];
      }
      argv[1][mountPoint.length()] = '\0';

      fuse_oper.getattr  = this->getattr; 
      fuse_oper.readdir  = this->readdir;                                                    
      fuse_oper.open   = this->open;                                                         
      fuse_oper.read   = this->read;                                                         

      int j = fuse_main(2, argv, &fuse_oper, NULL);
    };
};
