#include <fuse.h>                                                                      
#include <errno.h> 
#include <fcntl.h>  
#include <string.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "fs/stegfs.h"
#include "common/logging.h"
#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"

SteganographicFileSystem *SteganographicFileSystem::_instance = NULL;

SteganographicFileSystem::SteganographicFileSystem(VideoDecoder *decoder, SteganographicAlgorithm *alg,
    bool performance): decoder(decoder), alg(alg), performance(performance) {
  this->log = new Logger("/tmp/test.txt", false);

  Chunk *headerFrame = this->decoder->getFrame(0);
  char headerSig[4] = {0,0,0,0};
  this->alg->extract(headerFrame->getFrameData(), headerSig, 4, 0);
  printf("header: %.4s\n", headerSig);
  if (strncmp(headerSig, "STEG", 4) != 0) {
    printf("Could not read header. Has this video been formated?\n");
    exit(0);
  }

  char algE[4] = {0,0,0,0};
  this->alg->extract(headerFrame->getFrameData(), algE, 4, 4 * 8);

  union {
    uint32_t num;
    char byte[4];
  } headerBytes;
  headerBytes.num = 0;
  this->alg->extract(headerFrame->getFrameData(), headerBytes.byte, 4, 8 * 8);
  printf("Total headerbytes: %d\n", headerBytes.num);

  char *headerData = (char *)calloc(sizeof(char), headerBytes.num);
  this->alg->extract(headerFrame->getFrameData(), headerData, headerBytes.num, 12 * 8);
  this->readHeader(headerData, headerBytes.num); 
};

void SteganographicFileSystem::readHeader(char *headerBytes, int byteC) {
  int nextFrame = 1;
  int nextOffset = 0;
  int offset = 0;
  int i = 0;
  while (offset < byteC) {
    i = 0;
    while (headerBytes[offset + i++] != '\0') {}
    // offset + i is now on the end of the file name
    // i is the length of the string
    char *name = (char *)malloc(sizeof(char) * i);
    memcpy(name, headerBytes + offset, i);
    printf("File name: %s\n", name);
    std::string fileName((const char *)name);
    free(name);
    union {
      uint32_t num;
      char byte[4];
    } triples;
    triples.num = 0;
    memcpy(&triples, headerBytes + offset + i, 4);
    printf("File has %u triples\n", triples.num);
    int j = 0;
    int fileSize = 0;
    this->fileIndex[fileName.c_str()] = std::vector<tripleT>();
    for (j = 0; j < triples.num; j ++) {
      struct tripleT triple;
      memcpy(&triple, headerBytes + offset + i + j*sizeof(tripleT) + 4, sizeof(tripleT)); 
      this->fileIndex[fileName.c_str()].push_back(triple);
      // Work out where we should start writing i.e. largest frame + offset
      if (triple.frame > nextFrame) {
        nextFrame = triple.frame;
        nextOffset = triple.offset + triple.bytes;
      } else if (triple.frame == nextFrame) {
        if (triple.offset + triple.bytes > nextOffset) {
          nextOffset = triple.offset + triple.bytes;
        }
      }
      fileSize += triple.bytes;
    }
     
    printf("Final filesize: %d\n", fileSize);
    this->fileSizes[fileName.c_str()] = fileSize;        
    offset += i + j*sizeof(tripleT) + 4;
  }

  this->decoder->setNextFrameOffset(nextFrame, nextOffset);
};

SteganographicFileSystem *SteganographicFileSystem::Instance() {
  return SteganographicFileSystem::_instance;
};

void SteganographicFileSystem::Set(SteganographicFileSystem *i) {
  SteganographicFileSystem::_instance = i;
};

int SteganographicFileSystem::getattr(const char *path, struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  } else {
    for (auto kv : this->fileSizes) {
      if (strcmp(path, kv.first.c_str()) == 0) {
        stbuf->st_mode = S_IFREG | 0755;
        stbuf->st_nlink = 1;
        stbuf->st_size = kv.second;
        return 0;
      }
    }
  }
  // Requested file not in the filesystem...
  return -ENOENT;
};

int SteganographicFileSystem::utime(const char *path, struct utimbuf *ubuf) {
  // This implementation is needed
  return 0; 
};

int SteganographicFileSystem::access(const char *path, int mask) {
  // Let everyone access everything
  return 0;
};

int SteganographicFileSystem::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  if (strcmp(path, "/") != 0) {
    return -ENOENT;
  }

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  for (auto kv : this->fileSizes) {
    filler(buf, kv.first.c_str() + 1, NULL, 0);
  }
  return 0;
};

int SteganographicFileSystem::open(const char *path, struct fuse_file_info *fi) {
  // Just let everyone open everything
  return 0;
};

int SteganographicFileSystem::create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  // write new file to the header here 
  this->fileSizes[path] = 0;
  this->fileIndex[path] = std::vector<struct tripleT>();
  return 0;
};

int SteganographicFileSystem::read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  printf("Read called: size: %lu, offset: %jd\n", size, (intmax_t)offset);
  printf("Read called: size: %lu, offset: %jd\n", size, (intmax_t)offset);
  std::unordered_map<std::string, int>::const_iterator file = this->fileSizes.find(path);

  if (file == this->fileSizes.end()) {
    return -ENOENT;
  }

  if (size + offset > file->second) {
    size = file->second - offset;
  }
  if (offset > file->second) {
    return -ENOENT;
  }
  std::vector<tripleT> triples = this->fileIndex[path];

  int bytesWritten = 0;
  int readBytes = 0;
  int i = 0;
  for (struct tripleT t : triples) {
    if (readBytes + t.bytes >= offset) {
      while (bytesWritten < size) {
        struct tripleT t1 = triples.at(i);
        int chunkOffset = offset - readBytes;
        int bytesLeftInChunk = t1.bytes - chunkOffset;
        Chunk *c = this->decoder->getFrame(t1.frame); 
        if (size - bytesWritten < bytesLeftInChunk) {
          printf("\e[1A"); 
          printf("\e[0K\rExtracting: bytes: %lu, offset: %d\n", size-bytesWritten, t1.offset+chunkOffset);
          if (chunkOffset == 0) {
            this->alg->extract(c->getFrameData(), buf + bytesWritten, size - bytesWritten, (t1.offset + chunkOffset) * 8);
          } else {
            char *temp = (char *)malloc(t1.bytes*sizeof(char));
            this->alg->extract(c->getFrameData(), temp, t1.bytes, t1.offset * 8);
            memcpy(buf + bytesWritten, temp + chunkOffset, size - bytesWritten);
            free(temp);
          }
          return size;
        }
        printf("\e[1A"); 
        printf("\e[0K\rExtracting: bytes: %d, offset: %d\n", bytesLeftInChunk, t1.offset + chunkOffset);
        if (chunkOffset == 0) {
          this->alg->extract(c->getFrameData(), buf + bytesWritten, bytesLeftInChunk, (t1.offset + chunkOffset) * 8);
        } else {
          char *temp = (char *)malloc(t1.bytes*sizeof(char));
          this->alg->extract(c->getFrameData(), temp, t1.bytes, t1.offset * 8);
          memcpy(buf + bytesWritten, temp + chunkOffset, bytesLeftInChunk);
          free(temp);
        }
        bytesWritten += bytesLeftInChunk;
        // just to 0 chunkOffset from here on
        // TODO find a nicer way of doing this
        readBytes = offset;
        i ++;
      }
      return size;
    } else {
      readBytes += t.bytes;
      i ++;
    }
  }
  return -ENOENT;
};

//117MB limit if using 4096 chunks with a singe header frame
int SteganographicFileSystem::write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  //printf("Write called: path: %s, size: %zu, offset: %jd\n", path, size, (intmax_t)offset);  

  // Attempt to find the correct chunk
  bool needMoreChunks = true;
  int byteCount = 0;
  int bytesWritten = 0;

  for (struct tripleT t : this->fileIndex[path]) {
    if (byteCount + t.bytes > offset) {
      int chunkOffset = offset - byteCount;
      int bytesLeftInChunk = t.bytes - chunkOffset;
      if (bytesLeftInChunk + bytesWritten >= size) {
        // This chunk will finish it
        int toWrite = size - bytesWritten;
        this->alg->embed(this->decoder->getFrame(t.frame)->getFrameData(), (char *)(buf + bytesWritten), toWrite, (chunkOffset + t.offset) * 8);
        this->decoder->getFrame(t.frame)->setDirty(); 
        t.bytes = toWrite;
        needMoreChunks = false;
        break;
      }
      // Otherwise we can just write into the entire chunk
      this->alg->embed(this->decoder->getFrame(t.frame)->getFrameData(), (char *)(buf + bytesWritten), bytesLeftInChunk, (chunkOffset + t.offset) * 8);
      this->decoder->getFrame(t.frame)->setDirty(); 
      bytesWritten += bytesLeftInChunk;
      // Force chunkOffset to be 0 next time round
      byteCount = offset;  
    } else {
      byteCount += t.bytes;
    }
  }
  
  if (needMoreChunks == true) {
    // Need to allocate some new chunks
    while (bytesWritten < size) {
      int nextFrame = 0;
      int nextOffset = 0;
      this->decoder->getNextFrameOffset(&nextFrame, &nextOffset);  
      
      struct tripleT triple;
      int bytesLeftInFrame = this->decoder->frameSize() - nextOffset * 8;  
      // *8 since it takes 8 bytes to embed one byte
      if ((size - bytesWritten) * 8 < bytesLeftInFrame) {
        triple.bytes = size - bytesWritten;
        triple.frame = nextFrame;
        triple.offset = nextOffset;
        printf("\e[1A"); 
        printf("\e[0K\rEmbeding, nextFrame: %d, size: %zu, nextOffset: %d\n", nextFrame, size-bytesWritten, nextOffset * 8);
        this->alg->embed(this->decoder->getFrame(nextFrame)->getFrameData(), (char *)(buf + bytesWritten), triple.bytes, nextOffset * 8);
        this->decoder->getFrame(nextFrame)->setDirty(); 
        this->fileIndex[path].push_back(triple);
        this->decoder->setNextFrameOffset(nextFrame, nextOffset + size-bytesWritten);
        bytesWritten += size-bytesWritten;
      } else {
        // Write all bytes left in frame and go around again
        triple.bytes = bytesLeftInFrame / 8;
        triple.frame = nextFrame;
        triple.offset = nextOffset;
        printf("\e[1A"); 
        printf("\e[0K\rEmbeding, nextFrame: %d, size: %d, nextOffset: %d\n", nextFrame, bytesLeftInFrame / 8, nextOffset * 8);
        this->alg->embed(this->decoder->getFrame(nextFrame)->getFrameData(), (char *)(buf + bytesWritten), triple.bytes, nextOffset * 8);
        this->decoder->getFrame(nextFrame)->setDirty(); 
        this->fileIndex[path].push_back(triple);
        bytesWritten += bytesLeftInFrame / 8;
        this->decoder->setNextFrameOffset(nextFrame + 1, 0);
      }
    }
  }

  if (offset == 0) {
    this->fileSizes[path] = size;
  } else {
    this->fileSizes[path] += size;
  }

  return size;
};

int SteganographicFileSystem::fsync(const char *path, int datasync, struct fuse_file_info *fi) {
  // Flush any dirty frames we're holding
  printf("fsync called: datasync: %d\n", datasync);
  return 0;
};

int SteganographicFileSystem::truncate(const char *path, off_t newsize) {
  // This implementation is needed
  return 0;
};

void SteganographicFileSystem::writeHeader() {
  char *header = (char *)malloc(this->decoder->frameSize() * sizeof(char));
  int headerBytes = 0;
  int offset = 0;
  Chunk *headerFrame = this->decoder->getFrame(0);
  for (auto f : this->fileIndex) {
    memcpy(header + offset, (char *)f.first.c_str(), f.first.length()+1);
    offset += f.first.length() + 1;
    int triples = f.second.size();
    memcpy(header + offset, (char *)&triples, 4);
    offset += 4;
    
    // Embed all the triples
    for (struct tripleT t : f.second) {
      memcpy(header + offset, (char *)&t, sizeof(tripleT));
      offset += sizeof(tripleT);
    }
    headerBytes += f.first.length() + 1;
    headerBytes += 4;
    headerBytes += 4*3*f.second.size();
  }
  this->alg->embed(headerFrame->getFrameData(), (char *)&headerBytes, 4, (4+4) * 8); 
  this->alg->embed(headerFrame->getFrameData(), header, headerBytes, (4+4+4) * 8); 
  headerFrame->setDirty();
};

int SteganographicFileSystem::unlink(const char *path) {
  printf("Unlink called with path: %s\n", path);
  this->fileIndex.erase(this->fileIndex.find(path));
  this->fileSizes.erase(this->fileSizes.find(path));
  return 0;
};

int SteganographicFileSystem::flush(const char *path, struct fuse_file_info *fi) {
  printf("flush called: %s\n", path);
  if (!this->performance) {
    this->decoder->writeBack();
  }
  return 0; 
};
