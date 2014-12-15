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
#include "common/progress_bar.h"
#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"

SteganographicFileSystem *SteganographicFileSystem::_instance = NULL;

SteganographicFileSystem::SteganographicFileSystem(VideoDecoder *decoder, SteganographicAlgorithm *alg,
    bool performance): decoder(decoder), alg(alg), performance(performance) {
  this->log = new Logger("/tmp/test.txt", false);

  Chunk *headerFrame = this->decoder->getFrame(0);
  char headerSig[4] = {0,0,0,0};
  this->alg->extract(headerFrame, headerSig, 4, 0);
  printf("Header: %.4s\n", headerSig);
  if (strncmp(headerSig, "STEG", 4) != 0) {
    printf("Could not read header. Has this video been formated?\n");
    exit(0);
  }

  char algE[4] = {0,0,0,0};
  this->alg->extract(headerFrame, algE, 4, 4 * 8);

  char capacity = 0;
  this->alg->extract(headerFrame, &capacity, 4, 8 * 8);
  this->decoder->setCapacity(capacity);

  int headerBytes = 0;
  this->alg->extract(headerFrame, (char *)&headerBytes, 4, 9 * 8);
  printf("Total headerbytes: %d\n", headerBytes);

  char *headerData = (char *)malloc(sizeof(char) * headerBytes);
  this->alg->extract(headerFrame, headerData, headerBytes, 13 * 8);
  this->readHeader(headerData, headerBytes); 
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
    uint32_t triples = 0;
    memcpy(&triples, headerBytes + offset + i, 4);
    printf("File has %u triples\n", triples);
    int j = 0;
    int fileSize = 0;
    this->fileIndex[fileName.c_str()] = std::vector<tripleT>();
    for (j = 0; j < triples; j ++) {
      struct tripleT triple;
      memcpy(&triple, headerBytes + offset + i + j*sizeof(tripleT) + 4, sizeof(tripleT)); 
      printf("Triple: frame: %d, offset: %d, bytes: %d\n", triple.frame, triple.offset, triple.bytes);
      this->fileIndex[fileName.c_str()].push_back(triple);
      // Work out where we should start writing i.e. largest frame + offset
      if (triple.frame > nextFrame) {
        nextFrame = triple.frame;
        nextOffset = triple.offset + triple.bytes;
        if (nextOffset > this->decoder->frameSize() / 8) {
          // Chunk spanning multiple frames...
          nextFrame += nextOffset / (this->decoder->frameSize() / 8);
          nextOffset %= (this->decoder->frameSize() / 8); 
        }
      } else if (triple.frame == nextFrame) {
        if (triple.offset + triple.bytes > nextOffset) {
          nextOffset = triple.offset + triple.bytes;
          if (nextOffset > this->decoder->frameSize() / 8) {
            // Chunk spanning multiple frames...
            nextFrame += nextOffset / (this->decoder->frameSize() / 8);
            nextOffset %= (this->decoder->frameSize() / 8); 
          }
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
  printf("get attr: %s\n", path);
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
        printf("attr retuneed\n");
        return 0;
      }
    }
    for (auto kv : this->dirs) {
      if (strcmp(path, kv.first.c_str()) == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        printf("attr dir rred\n");
        return 0;
      }
    }
  }
  printf("somehow got here?\n");
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

int SteganographicFileSystem::mkdir(const char *path, mode_t mode) {
  printf("mkdir called: %s\n", path);
  printf("mkdir called: %s\n", path);
  this->dirs[path] = true;
  return 0;
}

int SteganographicFileSystem::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  //if (strcmp(path, "/") != 0) {
  //  return -ENOENT;
  //}
  
  printf("path: %s\n", path);

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  std::string prefix = path;
  int prefixSlash = std::count(prefix.begin(), prefix.end(), '/');
  for (auto kv : this->fileSizes) {
    printf("file: %s\n", kv.first.c_str());
    int nameSlash = std::count(kv.first.begin(), kv.first.end(), '/');
    if (prefix == "/") {
      if (nameSlash == 1) {
        printf("OK?? / %s\n", kv.first.c_str() + 1);
        filler(buf, kv.first.c_str() + 1, NULL, 0);
      } 
    } else if (kv.first.substr(0, prefix.size()) == prefix && prefixSlash == nameSlash - 1) {
      std::string name = kv.first.substr(kv.first.find_last_of("/") + 1);
      printf("OK??: %s\n", name.c_str());
      filler(buf, name.c_str(), NULL, 0);
    }
  }
  for (auto kv : this->dirs) {
    printf("dir: %s\n", kv.first.c_str());

    int nameSlash = std::count(kv.first.begin(), kv.first.end(), '/');
    if (prefix == "/") {
      if (nameSlash == 1) {
        filler(buf, kv.first.c_str() + 1, NULL, 0); 
        printf("adding dir /: %a\n", kv.first.c_str() + 1);
      }
    } else if (kv.first.substr(0, prefix.size()) == prefix && prefixSlash == nameSlash - 1) {
      std::string name = kv.first.substr(kv.first.find_last_of("/") + 1);
      filler(buf, name.c_str(), NULL, 0);
      printf("adding dir: %s\n", name.c_str());
    }
  }
  return 0;
};

int SteganographicFileSystem::open(const char *path, struct fuse_file_info *fi) {
  // Just let everyone open everything
  return 0;
};

int SteganographicFileSystem::create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  printf("created called: %s\n", path);
  printf("created called: %s\n", path);
  this->fileSizes[path] = 0;
  this->fileIndex[path] = std::vector<struct tripleT>();
  return 0;
};

int SteganographicFileSystem::read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  //printf("Read called: size: %lu, offset: %jd\n", size, (intmax_t)offset);
  std::unordered_map<std::string, int>::const_iterator file = this->fileSizes.find(path);

  if (file == this->fileSizes.end() || offset > file->second) {
    return -ENOENT;
  }

  if (size + offset > file->second) {
    size = file->second - offset;
  }

  this->mux.lock();
  std::vector<tripleT> triples = this->fileIndex[path];
  int bytesWritten = 0;
  int readBytes = 0;
  int i = 0;
  for (struct tripleT t : triples) {
    if (readBytes + t.bytes > offset) {
      while (bytesWritten < size) {
        struct tripleT t1 = triples.at(i);
        int frameSizeBytes = this->decoder->frameSize() / 8;
        bool spansMultipleFrames = ((int)t1.bytes + (int)t1.offset) > frameSizeBytes;
        if (spansMultipleFrames) {
          // This chunk spans multiple frames, deal with it seperatly
          // chunkOffset is frame relative to either the top of the frame, or top of the chunk
          int chunkOffset, actualFrame, chunkBytesInThisFrame;
          bool firstFrame = (offset - readBytes) < frameSizeBytes - t1.offset;
          if (firstFrame) {
            chunkOffset = offset - readBytes;
            actualFrame = t1.frame;
            chunkBytesInThisFrame = frameSizeBytes - (chunkOffset + t1.offset); 
          } else {
            // tmpOffset is the offset from the top of the second frame of the chunk
            // i.e. if tmpOffset == 0, we would be exactly at the top of the next frame
            int tmpOffset = offset - readBytes - (frameSizeBytes - t1.offset);
            chunkOffset = tmpOffset % frameSizeBytes;
            actualFrame = t1.frame + (tmpOffset / frameSizeBytes) + 1;
            chunkBytesInThisFrame = frameSizeBytes - chunkOffset;
            t1.offset = 0;
          }
          while (bytesWritten < t1.bytes) {
            Chunk *c = this->decoder->getFrame(actualFrame++); 
            if (size - bytesWritten <= chunkBytesInThisFrame) {
              if (chunkOffset == 0) {
                this->alg->extract(c, buf + bytesWritten, size - bytesWritten, t1.offset * 8);
              } else {
                char *temp = (char *)malloc((frameSizeBytes - t1.offset) * sizeof(char));
                this->alg->extract(c, temp, frameSizeBytes - t1.offset, t1.offset * 8);
                memcpy(buf + bytesWritten, temp + chunkOffset, size - bytesWritten);
                free(temp);
              }
              delete c;
              this->mux.unlock();
              return size;
            }

            if (chunkOffset == 0) {
              this->alg->extract(c, buf + bytesWritten, chunkBytesInThisFrame, t1.offset * 8);
            } else {
              char *temp = (char *)malloc((frameSizeBytes - t1.offset) * sizeof(char));
              this->alg->extract(c, temp, frameSizeBytes - t1.offset, t1.offset * 8);
              memcpy(buf + bytesWritten, temp + chunkOffset, chunkBytesInThisFrame);
              free(temp);
            }
            bytesWritten += chunkBytesInThisFrame;
            chunkOffset = 0;
            t1.offset = 0;
            chunkBytesInThisFrame = frameSizeBytes;
            delete c;
          } 
        } else {
          // This is a standard chunk, still need 2 cases for when chunkoffset
          // is in the middle of a chunk due to encrypting sequences
          int chunkOffset = offset - readBytes;
          int bytesLeftInChunk = t1.bytes - chunkOffset;
          Chunk *c = this->decoder->getFrame(t1.frame); 
          if (size - bytesWritten <= bytesLeftInChunk) {
            printf("\e[1A"); 
            printf("\e[0K\rExtracting: bytes: %lu, offset: %d\n", size-bytesWritten, t1.offset + chunkOffset);
            if (chunkOffset == 0) {
              this->alg->extract(c, buf + bytesWritten, size - bytesWritten, t1.offset * 8);
            } else {
              char *temp = (char *)malloc(t1.bytes * sizeof(char));
              this->alg->extract(c, temp, t1.bytes, t1.offset * 8);
              memcpy(buf + bytesWritten, temp + chunkOffset, size - bytesWritten);
              free(temp);
            }
            delete c;
            this->mux.unlock();
            return size;
          }
          printf("\e[1A"); 
          printf("\e[0K\rExtracting bytes: %d, offset: %d\n", bytesLeftInChunk, t1.offset + chunkOffset);
          if (chunkOffset == 0) {
            this->alg->extract(c, buf + bytesWritten, bytesLeftInChunk, t1.offset * 8);
          } else {
            char *temp = (char *)malloc(t1.bytes * sizeof(char));
            this->alg->extract(c, temp, t1.bytes, t1.offset * 8);
            memcpy(buf + bytesWritten, temp + chunkOffset, bytesLeftInChunk);
            free(temp);
          }
          delete c;
          bytesWritten += bytesLeftInChunk;
          // Just to 0 chunkOffset from here on
          readBytes = offset;
          i ++;
        }       
      }
      this->mux.unlock();
      return size;
    } else {
      readBytes += t.bytes;
      i ++;
    }
  }
  this->mux.unlock();
  return -ENOENT;
};

int SteganographicFileSystem::write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  this->mux.lock();
  //printf("Write called: path: %s, size: %zu, offset: %jd\n", path, size, (intmax_t)offset);  

  // Attempt to find the correct chunk
  bool needMoreChunks = true;
  int byteCount = 0;
  int bytesWritten = 0;

  for (struct tripleT t : this->fileIndex[path]) {
    if (byteCount + t.bytes > offset) {
      int chunkOffset = offset - byteCount;
      int bytesLeftInChunk = t.bytes - chunkOffset;
      Chunk *c = this->decoder->getFrame(t.frame);
      if (bytesLeftInChunk + bytesWritten >= size) {
        // This chunk will finish it
        int toWrite = size - bytesWritten;
        this->alg->embed(c, (char *)(buf + bytesWritten), toWrite, (chunkOffset + t.offset) * 8);
        this->decoder->getFrame(t.frame)->setDirty(); 
        t.bytes = toWrite;
        needMoreChunks = false;
        delete c;
        break;
      }
      // Otherwise we can just write into the entire chunk
      this->alg->embed(c, (char *)(buf + bytesWritten), bytesLeftInChunk, (chunkOffset + t.offset) * 8);
      this->decoder->getFrame(t.frame)->setDirty(); 
      bytesWritten += bytesLeftInChunk;
      // Force chunkOffset to be 0 next time round
      byteCount = offset;  
      delete c;
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
        printf("\e[0K\rEmbeding, nextFrame: %d, size: %zu, nextOffset: %d\n",
            nextFrame, size-bytesWritten, nextOffset * 8);
        Chunk *c = this->decoder->getFrame(nextFrame);
        this->alg->embed(c, (char *)(buf + bytesWritten), triple.bytes, nextOffset * 8);
        c->setDirty(); 
        delete c;
        this->fileIndex[path].push_back(triple);
        this->decoder->setNextFrameOffset(nextFrame, nextOffset + size-bytesWritten);
        bytesWritten += size-bytesWritten;
      } else {
        // Write all bytes left in frame and go around again
        triple.bytes = bytesLeftInFrame / 8;
        triple.frame = nextFrame;
        triple.offset = nextOffset;
        printf("\e[1A"); 
        printf("\e[0K\rEmbeding, nextFrame: %d, size: %d, nextOffset: %d\n",
            nextFrame, bytesLeftInFrame / 8, nextOffset * 8);
        Chunk *c = this->decoder->getFrame(nextFrame);
        this->alg->embed(c, (char *)(buf + bytesWritten), triple.bytes, nextOffset * 8);
        c->setDirty(); 
        delete c;
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
  this->mux.unlock();
  return size;
};

int SteganographicFileSystem::truncate(const char *path, off_t newsize) {
  // This implementation is needed
  return 0;
};

void SteganographicFileSystem::writeHeader() {
  this->compactHeader();
  char *header = (char *)malloc(this->decoder->frameSize() * sizeof(char));
  int headerBytes = 0;
  int offset = 0;
  Chunk *headerFrame = this->decoder->getFrame(0);
  for (auto f : this->fileIndex) {
    memcpy(header + offset, (char *)f.first.c_str(), f.first.length()+1);
    offset += f.first.length() + 1;
    int triples = f.second.size();
    char *tripleOffset = header + offset;
    // Fill this in later
    offset += 4;
    
    // Embed all the triples
    int embedded = 0;
    for (struct tripleT t : f.second) {
      if (offset > (this->decoder->frameSize() / 8) - 50) break;
      memcpy(header + offset, (char *)&t, sizeof(tripleT));
      offset += sizeof(tripleT);
      embedded ++;
    }
    memcpy(tripleOffset, (char *)&embedded, 4);

    headerBytes += f.first.length() + 1;
    headerBytes += 4;
    headerBytes += sizeof(tripleT)*embedded;
    if (offset > (this->decoder->frameSize() / 8) - 50) break;
  }
  int tmp = headerBytes;
  this->alg->embed(headerFrame, (char *)&headerBytes, 4, (4+4+1) * 8); 
  this->alg->embed(headerFrame, header, tmp, (4+4+4+1) * 8); 
  headerFrame->setDirty();
  free(header);
  delete headerFrame;
};

int SteganographicFileSystem::unlink(const char *path) {
  this->fileIndex.erase(this->fileIndex.find(path));
  this->fileSizes.erase(this->fileSizes.find(path));
  return 0;
};

void SteganographicFileSystem::compactHeader() {
  this->mux.lock();
  printf("Compacting header...\n");
  for (auto f : this->fileIndex) {
    int i = 0;
    std::unordered_map<int, std::vector<int> > chunkOffsets = std::unordered_map<int, std::vector<int> >();
    int size = f.second.size();
    while (i < size - 1) {
      struct tripleT current = f.second.at(i);
      struct tripleT next = f.second.at(i+1);
      if (current.frame == next.frame && current.offset + current.bytes == next.offset) {
        chunkOffsets[i].push_back(next.offset);
        current.bytes += next.bytes;
        f.second[i] = current;
        f.second.erase(f.second.begin() + i+1);
        size --;
      } else {
        i ++;
      }
    }
   char *tmp = (char *)malloc(this->decoder->frameSize() * sizeof(char));
    for (i = 0; i < f.second.size(); i ++) {
      loadBar(i+1, f.second.size(), 50);
      if (chunkOffsets[i].size() != 0) {
        struct tripleT t = f.second[i];
        int bytesRead = 0;
        Chunk *c = this->decoder->getFrame(t.frame);
        for (int j : chunkOffsets[i]) {
          int relOffset = j - t.offset;
          this->alg->extract(c, tmp + bytesRead, relOffset - bytesRead, (t.offset + bytesRead) * 8);
          bytesRead += (relOffset - bytesRead);
        }
        this->alg->extract(c, tmp + bytesRead, t.bytes - bytesRead, (t.offset + bytesRead) * 8);
        this->alg->embed(c, tmp, t.bytes, t.offset * 8);
        delete c;
      }
    }
    free(tmp);
    i = 0;
    // TODO: don't do the next bit if file size > 2GB...
    int framesAhead = 1;
    size = f.second.size();
    while (i < size - 1) {
      struct tripleT current = f.second.at(i);
      struct tripleT next = f.second.at(i+1);
      // TODO: This wont' work for the case where 2 spanning chunks sandwich a normal chunk...
      if (next.offset == 0 && current.offset + current.bytes == (this->decoder->frameSize() / 8) * framesAhead) {
        current.bytes += next.bytes;
        f.second[i] = current;
        f.second.erase(f.second.begin() + i+1);
        size --;
        framesAhead ++;
      } else {
        i ++;
      }
    }
    this->fileIndex[f.first] = f.second;
  }
  this->decoder->getFrame(0)->setDirty();
  this->mux.unlock();
};

int SteganographicFileSystem::flush(const char *path, struct fuse_file_info *fi) {
  printf("Flush called: %s\n", path);
  if (!this->performance) {
    this->writeHeader();
    this->decoder->writeBack();
  }
  return 0; 
};
