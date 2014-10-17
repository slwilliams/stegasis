#ifndef stegfs_h
#define stegfs_h

#include <fuse.h>                                                                      
#include <errno.h> 
#include <fcntl.h>  
#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <unordered_map>
#include <vector>

#include "common/logging.h"
#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"

struct tripleT {
  char frame;
  char offset;
  char bytes;
};

class SteganographicFileSystem {
  private:
    VideoDecoder *decoder;
    SteganographicAlgorithm *alg;
    Logger *log;
    std::unordered_map<std::string, int> fileSizes; 
    std::unordered_map<std::string, std::vector<struct tripleT> > fileIndex; 
    
  public:
    static SteganographicFileSystem *_instance;
    static SteganographicFileSystem *Instance();
    static void Set(SteganographicFileSystem *i);
    SteganographicFileSystem(VideoDecoder *decoder, SteganographicAlgorithm *alg);

    int getattr(const char *path, struct stat *stbuf); 
    int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);    
    int open(const char *path, struct fuse_file_info *fi);
    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);   
    void readHeader(char *headerBytes, int byteC);
};
#endif
