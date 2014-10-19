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

  char *headerData = (char *)calloc(sizeof(char), headerBytes.num);
  this->alg->extract(headerFrame->getFrameData(), headerData, headerBytes.num, 12 * 8);
  int i = 0;
  for (i = 0; i < 26; i ++) {
    printf("%u,", headerData[i]);
  }
  printf("\n");
  this->readHeader(headerData, headerBytes.num); 
};

void SteganographicFileSystem::readHeader(char *headerBytes, int byteC) {
  if (byteC == 0) return;
  int offset = 0;
  int i = 0;
  while (offset < byteC) {
    while (headerBytes[offset + i++] != '\0') {}
    printf("i is: %d\n", i);
    // offset + i is now on the end of the file name
    // i is the length of the string
    char *name = (char *)malloc(sizeof(char) * i);
    memcpy(name, headerBytes + offset, i);
    printf("name of file: %.10s\n", name);
    std::string fileName((const char *)name);
    printf("string: %.10s\n", fileName.c_str());
    union {
      uint32_t num;
      char byte[4];
    } triples;
    triples.num = 0;
    memcpy(&triples, headerBytes + offset + i, 4);
    printf("triples: %u\n", triples.num);
    int j = 0;
    int fileSize = 0;
    this->fileIndex[fileName.c_str()] = std::vector<tripleT>();
    for (j = 0; j < triples.num; j ++) {
      struct tripleT triple;
      memcpy(&triple, headerBytes + offset + i + j*sizeof(tripleT) + 4, sizeof(tripleT)); 
      printf("triple: frame: %d, bytes: %d, off: %d\n", triple.frame, triple.bytes, triple.offset);
      this->fileIndex[fileName.c_str()].push_back(triple);
      fileSize += triple.bytes;
    }
     
    printf("filesize: %d\n", fileSize);
    this->fileSizes[fileName.c_str()] = fileSize;        
    offset += i + j*sizeof(tripleT) + 4;
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
  return 0; 
};

int SteganographicFileSystem::access(const char *path, int mask) {
  // Let everyone access everything
  return 0;
};

int SteganographicFileSystem::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  if (strcmp(path, "/") != 0)
    return -ENOENT;

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
  printf("readcalled: size: %lu, offset: %jd\n", size, (intmax_t)offset);


  std::unordered_map<std::string, int>::const_iterator file = this->fileSizes.find(path);

  if (file == this->fileSizes.end()) {
    return -ENOENT;
  }

  int toRead = file->second;
  if (size + offset >toRead) {
    size = toRead - offset;
  }
  if (offset > toRead) {
    printf("hee??\n");
    return -ENOENT;
  }
  printf("toread: %d\n", toRead);
  int out = size;
  if (offset + size > toRead) {
    // This requested chunk will go off the end
    out = file->second - offset;
  }
  std::vector<tripleT> triples = this->fileIndex[path];
//  char *fileBuf = (char *)calloc(sizeof(char), toRead);

  int fileOffset = 0;
  int bytesWritten = 0;
  int readBytes = 0;
  int i = 0;
  for (auto t : triples) {
    if (readBytes + t.bytes >= offset) {
      while (bytesWritten < size) {
        auto t1 = triples.at(i);
        int chunkOffset = offset - readBytes;
        int bytesLeftInChunk = t1.bytes - chunkOffset;
        if (size - bytesWritten < bytesLeftInChunk) {
          Chunk *c = this->decoder->getFrame(t1.frame); 
          this->alg->extract(c->getFrameData(), buf, size-bytesWritten, (t1.offset + chunkOffset) * 8);
          return size;
        }
        Chunk *c = this->decoder->getFrame(t1.frame); 
        this->alg->extract(c->getFrameData(), buf, t1.bytes, t1.offset * 8);
        bytesWritten += t1.bytes;
        // just to 0 chunkOffset from here on
        readBytes = offset;
        i ++;
      }
      return size;
    } else {
      readBytes += t.bytes;
      i ++;
    }
  }
//    printf("triple: frame: %u, bytes: %u, offset: %u\n", t.frame, t.bytes, t.offset);
  /*  Chunk *c = this->decoder->getFrame(t.frame); 
    bytesWritten += t.bytes;
    if (bytesWritten > toRead) {
      break;
    }
  }
  memcpy(buf, fileBuf + offset, out);
  free(fileBuf);

  return size;*/
};

//117MB limit if using 4096 chunks with a singe header frame
int SteganographicFileSystem::write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  // work out difference between current size and new write size
  // write upto using current space until none left
  // then start allocating new space

  printf("write; path: %.10s, size: %zu, offset: %jd\n", path, size, (intmax_t)offset);  

  // Attempt to find the correct chunk
  struct tripleT theChunk;
  bool gotChunk = false;
  int byteCount = 0;
  for (auto t : this->fileIndex[path]) {
    byteCount += t.bytes;
    if (byteCount > offset) {
      theChunk = t;
      gotChunk = true;
      break;
    }
  }

  if (gotChunk == false) {
    printf("in here\n");
    // Need to allocate all new chunks
    int bytesWritten = 0;
    while (bytesWritten < size) {
      int nextFrame = 0;
      int nextOffset = 0;
      this->decoder->getNextFrameOffset(&nextFrame, &nextOffset);  
      struct tripleT triple;
      int bytesLeftInFrame = this->decoder->frameSize() - nextOffset * 8;  
      printf("bytes left in frame: %d\n", bytesLeftInFrame);
      //bytesLeftInFrame = std::floor((double)bytesLeftInFrame / 8);
      //printf("bytes left in frame: %d\n", bytesLeftInFrame);
      // TODO replace this duplicate code
      if (size*8 < bytesLeftInFrame) {
        triple.bytes = size - bytesWritten;
        triple.frame = nextFrame;
        triple.offset = nextOffset;
        printf("about to embed, nextFrame: %d, size: %zu, nextOffset: %d\n", nextFrame, size-bytesWritten, nextOffset);
        this->alg->embed(this->decoder->getFrame(nextFrame)->getFrameData(), (char *)buf, size-bytesWritten, nextOffset * 8);
        printf("segging here?\n");
        this->fileIndex[path].push_back(triple);
        this->decoder->setNextFrameOffset(nextFrame, nextOffset + size-bytesWritten);
        bytesWritten += size-bytesWritten;
      } else {
        printf("HELLO!!!!\n");
        // Write all bytes left in frame and go around again
        triple.bytes = bytesLeftInFrame;
        triple.frame = nextFrame;
        triple.offset = nextOffset;
        this->alg->embed(this->decoder->getFrame(nextFrame)->getFrameData(), (char *)buf, bytesLeftInFrame/8, nextOffset * 8);
        this->fileIndex[path].push_back(triple);
        bytesWritten += bytesLeftInFrame;
        this->decoder->setNextFrameOffset(nextFrame + 1, 0);
      }
    }
  } else {
    // Can overwrite existsing chunks
  }

  if (offset == 0) {
    this->fileSizes[path] = size;
  } else {
    this->fileSizes[path] += size;
  }

  return size;













  // If offset is 0 we can overwrite fileSizes[path]
  // if it is not we should add on size think, never seen offset != 0 though?
  
  // TODO none of this code below will work when offset > 0

  // Need to inlcude offset here...
  int sizeDiff = size + offset - this->fileSizes[path];
  printf("filesizes: %d\n", this->fileSizes[path]);
  printf("sizeDiff: %d\n", sizeDiff);
  if (sizeDiff < 0) {
    // new file is smaller, we could possible remove triples here...
    int bytesWritten = 0;
    int usedTriples = 0;
    for (auto t : this->fileIndex[path]) {
      usedTriples ++;
      int bytesLeft = size - bytesWritten;
      printf("bytesleft: %d\n", bytesLeft);
      if (bytesLeft - t.bytes <= 0) {
        //this one will finish it
        printf("t.frame: %u\n", t.frame);
        Chunk *c = this->decoder->getFrame(t.frame);
        printf("byteswritten: %d, t.offset: %u", bytesWritten, t.offset);
        this->alg->embed(c->getFrameData(), (char *)(buf + bytesWritten), bytesLeft, t.offset * 8);
        t.bytes = bytesLeft;
        // All elements left in the iterator can be removed
        break;
      } else {
        // This chunk will not finish it, fill the entire thing up
        Chunk *c = this->decoder->getFrame(t.frame);
        this->alg->embed(c->getFrameData(), (char *)(buf + bytesWritten), t.bytes, t.offset * 8);
        bytesWritten += t.bytes;
      }
    }
    // Remove now possibly unsed triples
    this->fileIndex[path].resize(usedTriples);
  } else {
    // new file is larger will need new triples but will use existing ones first...
    if (this->fileIndex[path].size() == 0) {
      printf("new exisiting file :)\n");
      // This is a new file, pretty sure offset must == 0
      int bytesWritten = 0;
      while (bytesWritten < size) {
        int nextFrame = 0;
        int nextOffset = 0;
        this->decoder->getNextFrameOffset(&nextFrame, &nextOffset);  
        struct tripleT triple;
        int bytesLeftInFrame = this->decoder->frameSize() - nextOffset;  
        // TODO replace this duplicate code
        if (size < bytesLeftInFrame) {
          triple.bytes = size;
          triple.frame = nextFrame;
          triple.offset = nextOffset;
          this->alg->embed(this->decoder->getFrame(nextFrame)->getFrameData(), (char *)buf, size, nextOffset * 8);
          this->fileIndex[path].push_back(triple);
          bytesWritten += size;
        } else {
          // Write all bytes left in frame and go around again
          triple.bytes = bytesLeftInFrame;
          triple.frame = nextFrame;
          triple.offset = nextOffset;
          this->alg->embed(this->decoder->getFrame(nextFrame)->getFrameData(), (char *)buf, bytesLeftInFrame, nextOffset * 8);
          this->fileIndex[path].push_back(triple);
          bytesWritten += bytesLeftInFrame;
          this->decoder->setNextFrameOffset(nextFrame + 1, 0);
        }
      }
      printf("out loop, triple is: frame: %u, bytes: %u, offset: %u\n", this->fileIndex[path].front().frame, this->fileIndex[path].front().bytes, this->fileIndex[path].front().offset);
    } else {
      // This is an existsing file, need to fill in exsisting triples first
      // Since this is larger than existsing file we know we will always write
      // over all current triplles..
      // However we don't know size is > all current tripples....
      int bytesWritten = 0;
      for (auto t : this->fileIndex[path]) {
        if (bytesWritten + t.bytes > size) {
          // This one will do this chunk, doubt this will ever happen...
          this->alg->embed(this->decoder->getFrame(t.frame)->getFrameData(), (char *)buf, size - bytesWritten, t.offset * 8);
          t.bytes = size - bytesWritten;
          break;
        } 
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

int SteganographicFileSystem::truncate(const char *path, off_t newsize) {
  return 0;
};
