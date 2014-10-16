#include <fuse.h>                                                                      
#include <errno.h> 
#include <fcntl.h>  
#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <unordered_map>

#include "fs/stegfs.h"
#include "common/logging.h"
#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"

SteganographicFileSystem *SteganographicFileSystem::_instance = NULL;
static const char *hello_str = "Hello World1234!\n";
static const char *hello_path = "/hello";


SteganographicFileSystem::SteganographicFileSystem(VideoDecoder *decoder, SteganographicAlgorithm *alg): decoder(decoder), alg(alg) {
  this->log = new Logger("/tmp/test.txt", false);

  Chunk *headerFrame = this->decoder->getFrame(0);
  char headerSig[4] = {0,0,0,0};
  this->alg->extract(headerFrame->getFrameData(), headerSig, 4, 0);
  printf("header: %.4s\n", headerSig);

  char algE[4] = {0,0,0,0};
  this->alg->extract(headerFrame->getFrameData(), algE, 4, 4 * 8);
  printf("alg: %.4s\n", algE);

  union {
    uint32_t num;
    char byte[4];
  } headerBytes;
  headerBytes.num = 0;
  this->alg->extract(headerFrame->getFrameData(), headerBytes.byte, 4, 8 * 8);
  printf("headerbytes: %d\n", headerBytes.num);

  char *headerData = (char *)malloc(sizeof(char) * headerBytes.num);
  this->alg->extract(headerFrame->getFrameData(), headerData, headerBytes.num, 12 * 8);

  //TODO extract filename based on \0 char, get number of triples and extract them
  //repeate for other filenames
  this->readHeader(headerData, headerBytes.num); 
};

void SteganographicFileSystem::readHeader(char *headerBytes, int byteC) {
  if (byteC == 0) return;
  int offset = 0;
  int i = 0;
  while (offset < bytesC) {
    while (headerBytes[offset + i++] != '\0') {}
    // offset + i is now on the end of the file name
    // i is the length of the string
    char *name = (char *)malloc(sizeof(char) * i);
    memcpy(name, headerBytes + offset, i);
    std::string fileName((const char *)name);
    char triples = 0;
    memcpy(&triples, headerBytes + offset + i, 1);
    int j = 0;
    int fileSize = 0;
    for (j = 0; j < triples; j ++) {
      char triple[3];
      memcpy((char *)triple, headerBytes + offset + i + j*3 + 1, 3); 
      fileSize += triple[2];
    }
     
    this->fileSizes[fileName.c_str()] = fileSize;        
    offset += i + j*3 + 1;
  }
};

SteganographicFileSystem *SteganographicFileSystem::Instance() {
  return SteganographicFileSystem::_instance;
};

void SteganographicFileSystem::Set(SteganographicFileSystem *i) {
  SteganographicFileSystem::_instance = i;
};

int SteganographicFileSystem::getattr(const char *path, struct stat *stbuf) {
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
};

int SteganographicFileSystem::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  if (strcmp(path, "/") != 0)
    return -ENOENT;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  Chunk *headerFrame = this->decoder->getFrame(0);
  char *file = (char *)malloc(10);
  this->alg->extract(headerFrame->getFrameData(), file, 10, 8 * 8);
  filler(buf, file + 1, NULL, 0);
  return 0;
};

int SteganographicFileSystem::open(const char *path, struct fuse_file_info *fi) {
  if (strcmp(path, hello_path) != 0)
    return -ENOENT;

  if ((fi->flags & 3) != O_RDONLY)
    return -EACCES;

  return 0;
};

int SteganographicFileSystem::read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
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
};
