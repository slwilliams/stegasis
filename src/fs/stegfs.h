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
#include <mutex>

#include "common/logging.h"
#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"

using namespace std;

struct FileChunk {
  uint32_t frame;
  uint32_t offset;
  uint32_t bytes;
};

class SteganographicFileSystem {
  private:
    VideoDecoder *decoder;
    SteganographicAlgorithm *alg;

    mutex mux;
    bool performance;
    int headerBytesFrame;
    int headerBytesOffset;

    unordered_map<string, int> fileSizes; 
    unordered_map<string, vector<struct FileChunk> > fileIndex; 
    unordered_map<string, bool> dirs;

    void extract(int *initialFrame, int *initialOffset, int bytes, char *out);
    
  public:
    static SteganographicFileSystem *_instance;
    static SteganographicFileSystem *Instance();
    static void Set(SteganographicFileSystem *i);
    SteganographicFileSystem(VideoDecoder *decoder, SteganographicAlgorithm *alg, bool performance);

    int getattr(const char *path, struct stat *stbuf); 
    int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);    
    int open(const char *path, struct fuse_file_info *fi);
    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);   
    int write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
    int access(const char *path, int mask);
    int truncate(const char *path, off_t newsize);
    int create(const char *path, mode_t mode, struct fuse_file_info *fi);
    int utime(const char *path, struct utimbuf *ubuf);
    int unlink(const char *path);
    int flush(const char *path, struct fuse_file_info *fi);
    int mkdir(const char *path, mode_t mode);

    void writeHeader();
    void compactHeader();

    void readHeader(char *headerBytes, int byteC);
};
#endif
