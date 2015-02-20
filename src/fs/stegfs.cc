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
#include <utility>

#include "fs/stegfs.h"
#include "common/progress_bar.h"
#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"

using namespace std;

SteganographicFileSystem *SteganographicFileSystem::_instance = NULL;

void SteganographicFileSystem::extract(int *frame, int *offset, int bytes, char *out) {
  int bytesWritten = 0;
  pair<int, int> extractedWritten = make_pair(0, *offset);
  do {
    //printf("reading from frame: %d, offset: %d\n", *frame, extractedWritten.second);
    extractedWritten = this->alg->extract(this->decoder->getFrame(*frame), out+bytesWritten, bytes-bytesWritten, extractedWritten.second); 
    //printf("got: %d\n", extractedWritten.first);
    bytesWritten += extractedWritten.first;

    if(extractedWritten.second == 0) (*frame) ++;
  } while (bytesWritten != bytes); 
  (*offset) = extractedWritten.second;
};

//// <bytesWritten, offset>
SteganographicFileSystem::SteganographicFileSystem(VideoDecoder *decoder, SteganographicAlgorithm *alg,
    bool performance): decoder(decoder), alg(alg), performance(performance) {
  bool hiddenVolume = false;

  Frame *headerFrame = this->decoder->getHeaderFrame();
  pair<int, int> offset = make_pair(0,0);
  int frame = 0;

  char headerSig[4] = {0,0,0,0};
  this->extract(&frame, &offset.second, 4, headerSig);
  printf("Header: %.4s\n", headerSig);
  //printf("offset after heardersig: %d\n", offset.second);
  if (strncmp(headerSig, "STEG", 4) != 0) {
    this->decoder->setHiddenVolume(); 
    headerFrame = this->decoder->getHeaderFrame();
    offset = this->alg->extract(headerFrame, headerSig, 4, 0);
    if (strncmp(headerSig, "STEG", 4) != 0) {
      printf("Could not read header. Has this video been formated?\n");
      exit(0);
    }
    hiddenVolume = true;
  }
  printf("Header: %.4s\n", headerSig);

  char algE[4] = {0,0,0,0};
  this->extract(&frame, &offset.second, 4, algE);
  printf("Embedding Algorithm: %.4s\n", algE);

  char capacity = 0;
  this->extract(&frame, &offset.second, 1, &capacity);
  printf("Capacity: %d\n", capacity);

  int headerBytes = 0;
  //printf("offset before headberyets: %d, frame: %d\n", offset.second, frame); 
  this->headerBytesFrame = frame;
  this->headerBytesOffset = offset.second;
  this->extract(&frame, &offset.second, 4, (char *)&headerBytes);
  printf("Total headerbytes: %d\n", headerBytes);

  char *headerData = (char *)malloc(sizeof(char) * headerBytes);
  this->extract(&frame, &offset.second, headerBytes, headerData);
  this->readHeader(headerData, headerBytes); 
  this->decoder->setCapacity(capacity);

  if (hiddenVolume) {
    int currentFrameOffset, currentFrame;
    this->decoder->getNextFrameOffset(&currentFrame, &currentFrameOffset);
    if (currentFrame == 1 && currentFrameOffset == 0) {
      currentFrame = (this->decoder->getNumberOfFrames() / 2) + 1;
      this->decoder->setNextFrameOffset(currentFrame, 0);
    }
  }
};

SteganographicFileSystem *SteganographicFileSystem::Instance() {
  return SteganographicFileSystem::_instance;
};

void SteganographicFileSystem::Set(SteganographicFileSystem *i) {
  SteganographicFileSystem::_instance = i;
};

void SteganographicFileSystem::readHeader(char *headerBytes, int byteC) {
  int nextFrame = 50;
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
    string fileName((const char *)name);
    free(name);
    int32_t triples = 0;
    memcpy(&triples, headerBytes + offset + i, 4);
    printf("File has %d triples\n", triples);
    if (triples == -1) {
      // This file is a directory
      printf("Got a dir: %s\n", fileName.c_str());
      this->dirs[fileName] = true;
      // Advance offset past the dir name and the -1 header bytes
      offset += i + 4;
      continue;
    }
    int j = 0;
    int fileSize = 0;
    this->fileIndex[fileName.c_str()] = vector<FileChunk>();
    for (j = 0; j < triples; j ++) {
      struct FileChunk triple;
      memcpy(&triple, headerBytes + offset + i + j*sizeof(FileChunk) + 4, sizeof(FileChunk)); 
      printf("Triple: frame: %d, offset: %d, bytes: %d\n", triple.frame, triple.offset, triple.bytes);
      this->fileIndex[fileName.c_str()].push_back(triple);
      // Work out where we should start writing i.e. largest frame + offset
      // //TODO: This now doesn't work - can't just add offset onto chunk.bytes...
      if (triple.frame > nextFrame) {
        nextFrame = triple.frame;
        nextOffset = triple.offset + triple.bytes;
        if (nextOffset > this->decoder->getFrameSize() / 8) {
          // Chunk spanning multiple frames...
          nextFrame += nextOffset / (this->decoder->getFrameSize() / 8);
          nextOffset %= (this->decoder->getFrameSize() / 8); 
        }
      } else if (triple.frame == nextFrame) {
        if (triple.offset + triple.bytes > nextOffset) {
          nextOffset = triple.offset + triple.bytes;
          if (nextOffset > this->decoder->getFrameSize() / 8) {
            // Chunk spanning multiple frames...
            nextFrame += nextOffset / (this->decoder->getFrameSize() / 8);
            nextOffset %= (this->decoder->getFrameSize() / 8); 
          }
        }
      }
      fileSize += triple.bytes;
    }
     
    printf("Final filesize: %d\n", fileSize);
    this->fileSizes[fileName.c_str()] = fileSize;        
    offset += i + j*sizeof(FileChunk) + 4;
  }

  this->decoder->setNextFrameOffset(nextFrame, nextOffset);
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
    for (auto kv : this->dirs) {
      if (strcmp(path, kv.first.c_str()) == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
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

int SteganographicFileSystem::open(const char *path, struct fuse_file_info *fi) {
  // Let everyone open everything
  return 0;
};

int SteganographicFileSystem::access(const char *path, int mask) {
  // Let everyone access everything
  return 0;
};

int SteganographicFileSystem::truncate(const char *path, off_t newsize) {
  // This implementation is needed
  return 0;
};

int SteganographicFileSystem::mkdir(const char *path, mode_t mode) {
  this->dirs[path] = true;
  return 0;
};

int SteganographicFileSystem::create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  this->fileSizes[path] = 0;
  this->fileIndex[path] = vector<struct FileChunk>();
  return 0;
};

int SteganographicFileSystem::unlink(const char *path) {
  if (fileIndex.find(path) != fileIndex.end()) {
    this->fileIndex.erase(this->fileIndex.find(path));
    this->fileSizes.erase(this->fileSizes.find(path));
  } else {
    this->dirs.erase(this->dirs.find(path));
  }
  return 0;
};

int SteganographicFileSystem::flush(const char *path, struct fuse_file_info *fi) {
  if (!this->performance) {
    this->writeHeader();
    this->decoder->writeBack();
  }
  return 0; 
};

int SteganographicFileSystem::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  string prefix = path;
  int prefixSlash = count(prefix.begin(), prefix.end(), '/');
  for (auto kv : this->fileSizes) {
    int nameSlash = count(kv.first.begin(), kv.first.end(), '/');
    if (prefix == "/") {
      if (nameSlash == 1) {
        filler(buf, kv.first.c_str() + 1, NULL, 0);
      } 
    } else if (kv.first.substr(0, prefix.size()) == prefix && prefixSlash == nameSlash - 1) {
      string name = kv.first.substr(kv.first.find_last_of("/") + 1);
      filler(buf, name.c_str(), NULL, 0);
    }
  }
  for (auto kv : this->dirs) {
    int nameSlash = count(kv.first.begin(), kv.first.end(), '/');
    if (prefix == "/") {
      if (nameSlash == 1) {
        filler(buf, kv.first.c_str() + 1, NULL, 0); 
      }
    } else if (kv.first.substr(0, prefix.size()) == prefix && prefixSlash == nameSlash - 1) {
      string name = kv.first.substr(kv.first.find_last_of("/") + 1);
      filler(buf, name.c_str(), NULL, 0);
    }
  }
  return 0;
};

int SteganographicFileSystem::read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  this->mux.lock();

  unordered_map<string, int>::const_iterator file = this->fileSizes.find(path);
  
  if (file == this->fileSizes.end() || offset > file->second) {
    return -ENOENT;
  }

  if (size + offset > file->second) {
    size = file->second - offset;
  }

  vector<FileChunk> fileChunks = this->fileIndex[path];
  int bytesRead = 0, chunkNum = 0, bytesWritten = 0;
  
  for (struct FileChunk c : fileChunks) {
    if (bytesRead + c.bytes > offset) {
      break;
    } else {
      bytesRead += c.bytes;
      chunkNum ++;
    }
  }

  while (bytesWritten < size) {
    struct FileChunk chunk = fileChunks.at(chunkNum);
    int chunkOffset = offset - bytesRead;
    int bytesLeftInChunk = chunk.bytes - chunkOffset;

    //printf("chunk: frame: %d, bytes: %d, offset :%d\n\n", chunk.frame, chunk.bytes, chunk.offset);

    bytesLeftInChunk = min((int)(size - bytesWritten), bytesLeftInChunk);

    //printf("\e[1A"); 
    printf("\e[0K\rExtracting bytes: %d, offset: %d, frame: %d\n", bytesLeftInChunk, chunk.offset, chunk.frame);
    if (chunkOffset == 0) {
      this->extract((int *)&chunk.frame, (int *)&chunk.offset, bytesLeftInChunk, buf+bytesWritten);
    } else {
      char *temp = (char *)malloc(chunk.bytes * sizeof(char));
      this->extract((int *)&chunk.frame, (int *)&chunk.offset, chunk.bytes, temp);
      memcpy(buf + bytesWritten, temp + chunkOffset, bytesLeftInChunk);
      free(temp);
    }

    bytesWritten += bytesLeftInChunk;
    bytesRead = offset;
    chunkNum ++;
  }

  this->mux.unlock();
  return size;
};

/*int SteganographicFileSystem::read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  //printf("Read called: size: %lu, offset: %jd\n", size, (intmax_t)offset);
  //printf("Read called: size: %lu, offset: %jd\n", size, (intmax_t)offset);
  unordered_map<string, int>::const_iterator file = this->fileSizes.find(path);

  if (file == this->fileSizes.end() || offset > file->second) {
    return -ENOENT;
  }

  if (size + offset > file->second) {
    size = file->second - offset;
  }

  this->mux.lock();
  vector<FileChunk> triples = this->fileIndex[path];
  int bytesWritten = 0;
  int readBytes = 0;
  int i = 0;
  for (struct FileChunk t : triples) {
    if (readBytes + t.bytes > offset) {
      while (bytesWritten < size) {
        printf("looping!!\n\n");
        struct FileChunk t1 = triples.at(i);
        int frameSizeBytes = this->decoder->getFrameSize() / 8;
        bool spansMultipleFrames = ((int)t1.bytes + (int)t1.offset) > frameSizeBytes;
        if (spansMultipleFrames) {
          printf("some how in here??\n\n");
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
            Frame *f = this->decoder->getFrame(actualFrame++); 
            if (size - bytesWritten <= chunkBytesInThisFrame) {
              if (chunkOffset == 0) {
                this->alg->extract(f, buf + bytesWritten, size - bytesWritten, t1.offset * 8);
              } else {
                char *temp = (char *)malloc((frameSizeBytes - t1.offset) * sizeof(char));
                this->alg->extract(f, temp, frameSizeBytes - t1.offset, t1.offset * 8);
                memcpy(buf + bytesWritten, temp + chunkOffset, size - bytesWritten);
                free(temp);
              }
              delete f;
              this->mux.unlock();
              return size;
            }

            if (chunkOffset == 0) {
              this->alg->extract(f, buf + bytesWritten, chunkBytesInThisFrame, t1.offset * 8);
            } else {
              char *temp = (char *)malloc((frameSizeBytes - t1.offset) * sizeof(char));
              this->alg->extract(f, temp, frameSizeBytes - t1.offset, t1.offset * 8);
              memcpy(buf + bytesWritten, temp + chunkOffset, chunkBytesInThisFrame);
              free(temp);
            }
            bytesWritten += chunkBytesInThisFrame;
            chunkOffset = 0;
            t1.offset = 0;
            chunkBytesInThisFrame = frameSizeBytes;
            delete f;
          } 
        } else {
          printf("notmal\n");
          // This is a standard chunk, still need 2 cases for when chunkoffset
          // is in the middle of a chunk due to encrypting sequences
          int chunkOffset = offset - readBytes;
          int bytesLeftInChunk = t1.bytes - chunkOffset;
          Frame *f = this->decoder->getFrame(t1.frame); 
          if (size - bytesWritten <= bytesLeftInChunk) {
            printf("\e[1A"); 
            printf("\e[0K\rExtracting: bytes: %lu, offset: %d\n", size-bytesWritten, t1.offset + chunkOffset);
            printf("going in...\n\n");
            if (chunkOffset == 0) {
              this->alg->extract(f, buf + bytesWritten, size - bytesWritten, t1.offset);
            } else {
              char *temp = (char *)malloc(t1.bytes * sizeof(char));
              this->alg->extract(f, temp, t1.bytes, t1.offset);
              memcpy(buf + bytesWritten, temp + chunkOffset, size - bytesWritten);
              free(temp);
            }
            printf("out...\n\n");
            delete f;
            this->mux.unlock();
            return size;
          }
          printf("out...\n\n");
          printf("\e[1A"); 
          printf("\e[0K\rExtracting bytes: %d, offset: %d\n", bytesLeftInChunk, t1.offset + chunkOffset);
          printf("going in again...\n\n");
          if (chunkOffset == 0) {
            this->alg->extract(f, buf + bytesWritten, bytesLeftInChunk, t1.offset);
          } else {
            char *temp = (char *)malloc(t1.bytes * sizeof(char));
            this->alg->extract(f, temp, t1.bytes, t1.offset);
            memcpy(buf + bytesWritten, temp + chunkOffset, bytesLeftInChunk);
            free(temp);
          }
          printf("out again\n\n");
          delete f;
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
};*/

int SteganographicFileSystem::write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  this->mux.lock();
  //printf("Write called: path: %s, size: %zu, offset: %jd\n", path, size, (intmax_t)offset);  
  //printf("Write called: path: %s, size: %zu, offset: %jd\n", path, size, (intmax_t)offset);  
  int bytesWritten = 0, nextFrame = 0, nextOffset = 0; 

  this->decoder->getNextFrameOffset(&nextFrame, &nextOffset);  

  /*struct FileChunk triple;
  triple.frame = nextFrame;
  triple.offset = nextOffset;
  triple.bytes = 0;*/

  while (bytesWritten < size) {
    this->decoder->getNextFrameOffset(&nextFrame, &nextOffset);  

    struct FileChunk triple;
    triple.frame = nextFrame;
    triple.offset = nextOffset;
    triple.bytes = 0;
    
    //printf("\e[1A"); 
    printf(/*\e[0K\r*/"Embeding, nextFrame: %d, nextOffset: %d, bytesWritten: %d\n", nextFrame, nextOffset * 8, bytesWritten);
    int tmp = this->alg->embed(this->decoder->getFrame(nextFrame), (char *)(buf + bytesWritten), size-bytesWritten, nextOffset); 
    printf("got a tmp of: %d\n", tmp);
    triple.bytes += tmp;
    //bytesWritten += tmp;

    if (tmp != 0) {
      this->fileIndex[path].push_back(triple);
      // ------ tmp ------
      char *tmpData = (char *)malloc(sizeof(int) * tmp);
      this->alg->extract(this->decoder->getFrame(nextFrame), tmpData, tmp, 0);
      for (int i = 0; i < tmp; i ++) {
        if (tmpData[i] != (buf + bytesWritten)[i]) {
          printf("i: %d, input: %d, gotout: %d, tmp: %d\n", i, (buf + bytesWritten)[i], tmpData[i], tmp);
          abort();
        }
      }
    }
    printf("finished here\n");
    bytesWritten += tmp;
  }
  //printf("adding triple frame: %d, bytes:%d, offset: %d\n", triple.frame, triple.bytes, triple.offset);
  //this->fileIndex[path].push_back(triple);

  if (offset == 0) {
    this->fileSizes[path] = size;
  } else {
    this->fileSizes[path] += size;
  }
  this->mux.unlock();
  return size;
};

/*int SteganographicFileSystem::write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  this->mux.lock();
 // printf("Write called: path: %s, size: %zu, offset: %jd\n", path, size, (intmax_t)offset);  
//  printf("Write called: path: %s, size: %zu, offset: %jd\n", path, size, (intmax_t)offset);  

  // Attempt to find the correct chunk
  bool needMoreChunks = true;
  int byteCount = 0;
  int bytesWritten = 0;

  // This now doesn't work due to the single chunk per file thing...
  // if offset > framesize div it with framesize and add this on to the frame number i think
  for (struct FileChunk t : this->fileIndex[path]) {
    if (byteCount + t.bytes > offset) {
      int chunkOffset = offset - byteCount;
      int bytesLeftInChunk = t.bytes - chunkOffset;
      Frame *f = this->decoder->getFrame(t.frame);
      if (bytesLeftInChunk + bytesWritten >= size) {
        // This chunk will finish it
        int toWrite = size - bytesWritten;
        printf("\e[1A"); 
        printf("\e[0K\rEmbeding1_over, nextFrame: %d, size: %d, nextOffset: %d\n",
            t.frame, toWrite, (chunkOffset + t.offset) * 8);
        this->alg->embed(f, (char *)(buf + bytesWritten), toWrite, (chunkOffset + t.offset) * 8);
        this->decoder->getFrame(t.frame)->setDirty(); 
        t.bytes = toWrite;
        needMoreChunks = false;
        delete f;
        break;
      }
      // Otherwise we can just write into the entire chunk
      printf("\e[1A"); 
      printf("\e[0K\rEmbeding2_over, nextFrame: %d, size: %d, nextOffset: %d\n",
            t.frame, bytesLeftInChunk, (chunkOffset + t.offset) * 8);
      this->alg->embed(f, (char *)(buf + bytesWritten), bytesLeftInChunk, (chunkOffset + t.offset) * 8);
      this->decoder->getFrame(t.frame)->setDirty(); 
      bytesWritten += bytesLeftInChunk;
      // Force chunkOffset to be 0 next time round
      byteCount = offset;  
      delete f;
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
      
      struct FileChunk triple;
      int bytesLeftInFrame = this->decoder->getFrameSize() - nextOffset * 8;  
      // *8 since it takes 8 bytes to embed one byte
      if ((size - bytesWritten) * 8 < bytesLeftInFrame) {
        triple.bytes = size - bytesWritten;
        triple.frame = nextFrame;
        triple.offset = nextOffset;
        printf("\e[1A"); 
        printf("\e[0K\rEmbeding1, nextFrame: %d, size: %zu, nextOffset: %d\n",
            nextFrame, size-bytesWritten, nextOffset * 8);
        Frame *f = this->decoder->getFrame(nextFrame);
        this->alg->embed(f, (char *)(buf + bytesWritten), triple.bytes, nextOffset * 8);
        f->setDirty(); 
        delete f;
        this->fileIndex[path].push_back(triple);
        this->decoder->setNextFrameOffset(nextFrame, nextOffset + size-bytesWritten);
        bytesWritten += size-bytesWritten;
      } else {
        // Write all bytes left in frame and go around again
        triple.bytes = bytesLeftInFrame / 8;
        triple.frame = nextFrame;
        triple.offset = nextOffset;
        printf("\e[1A"); 
        printf("\e[0K\rEmbeding2, nextFrame: %d, size: %d, nextOffset: %d\n",
            nextFrame, triple.bytes, nextOffset * 8);
        Frame *f = this->decoder->getFrame(nextFrame);
        this->alg->embed(f, (char *)(buf + bytesWritten), triple.bytes, nextOffset * 8);
        f->setDirty(); 
        delete f;
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
};*/

void SteganographicFileSystem::writeHeader() {
  //this->compactHeader();
  // TODO: Calculate header bytes for this malloc call
  this->mux.lock();
  int oldFrameOffset, oldFrame;
  this->decoder->getNextFrameOffset(&oldFrame, &oldFrameOffset);

  char *header = (char *)malloc(this->decoder->getFrameSize() * sizeof(char) * 50);
  int headerBytes = 0, offset = 0;
  for (auto f : this->fileIndex) {
    // Copy the filename
    memcpy(header + offset, (char *)f.first.c_str(), f.first.length()+1);
    offset += f.first.length() + 1;
    headerBytes += f.first.length() + 1;

    // Copy the number of chunks
    int triples = f.second.size();
    memcpy(header + offset, (char *)&triples, 4);
    offset += 4;
    headerBytes += 4;
    
    // Copy all the chunks
    int embedded = 0;
    for (struct FileChunk t : f.second) {
      printf("embedding triple with frame: %d, bytes: %d, off: %d\n", t.frame, t.bytes, t.offset);
      memcpy(header + offset, (char *)&t, sizeof(FileChunk));
      offset += sizeof(FileChunk);
      headerBytes += sizeof(FileChunk); 
    }
  }
  // Copy dirs
  for (auto d : this->dirs) {
    memcpy(header + offset, (char *)d.first.c_str(), d.first.length()+1);
    offset += d.first.length() + 1;
    headerBytes += d.first.length() + 1;

    int dirTriples = -1;
    memcpy(header + offset, (char *)&dirTriples, 4);
    offset += 4;
    headerBytes += 4;
  }
  printf("Headerbytes: %d\n", headerBytes);

  int tmp = headerBytes;
  int currentFrame, currentOffset;
  this->decoder->setNextFrameOffset(this->headerBytesFrame, this->headerBytesOffset);

  int bytesWritten = 0;
  int bytesToWrite = 4;
  do {
    this->decoder->getNextFrameOffset(&currentFrame, &currentOffset);
    bytesWritten += alg->embed(this->decoder->getFrame(currentFrame), ((char *)&headerBytes)+bytesWritten, bytesToWrite-bytesWritten, currentOffset);
  } while (bytesWritten != bytesToWrite);

  bytesWritten = 0;
  bytesToWrite = tmp;
  do {
    this->decoder->getNextFrameOffset(&currentFrame, &currentOffset);
    if (currentFrame == 50) {
      printf("did get here\n\n");
      break;
    }
    bytesWritten += alg->embed(this->decoder->getFrame(currentFrame), header+bytesWritten, bytesToWrite-bytesWritten, currentOffset);
  } while (bytesWritten != bytesToWrite);
  free(header);

  this->decoder->setNextFrameOffset(oldFrame, oldFrameOffset);
  this->mux.unlock();
};

/*void SteganographicFileSystem::compactHeader() {
  this->mux.lock();
  printf("Compacting header...\n");
  for (auto f : this->fileIndex) {
    int i = 0;
    unordered_map<int, vector<int> > chunkOffsets = unordered_map<int, vector<int> >();
    int size = f.second.size();
    while (i < size - 1) {
      struct FileChunk current = f.second.at(i);
      struct FileChunk next = f.second.at(i+1);
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
    char *tmp = (char *)malloc(this->decoder->getFrameSize() * sizeof(char));
    for (i = 0; i < f.second.size(); i ++) {
      loadBar(i+1, f.second.size(), 50);
      if (chunkOffsets[i].size() != 0) {
        struct FileChunk t = f.second[i];
        int bytesRead = 0;
        Frame *f = this->decoder->getFrame(t.frame);
        for (int j : chunkOffsets[i]) {
          int relOffset = j - t.offset;
          this->alg->extract(f, tmp + bytesRead, relOffset - bytesRead, (t.offset + bytesRead) * 8);
          bytesRead += (relOffset - bytesRead);
        }
        this->alg->extract(f, tmp + bytesRead, t.bytes - bytesRead, (t.offset + bytesRead) * 8);
        this->alg->embed(f, tmp, t.bytes, t.offset * 8);
        delete f;
      }
    }
    free(tmp);
    i = 0;
    // TODO: don't do the next bit if file size > 2GB...
    int framesAhead = 1;
    size = f.second.size();
    while (i < size - 1) {
      struct FileChunk current = f.second.at(i);
      struct FileChunk next = f.second.at(i+1);
      // TODO: This wont' work for the case where 2 spanning chunks sandwich a normal chunk...
      if (next.offset == 0 && current.offset + current.bytes == (this->decoder->getFrameSize() / 8) * framesAhead) {
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
  this->decoder->getHeaderFrame()->setDirty();
  this->mux.unlock();
};*/
