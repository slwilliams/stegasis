#include <stdio.h>
#include <string.h>
#include <string>
#include <mutex> 
#include <math.h>

#include "common/progress_bar.h"
#include "video_decoder.h"

using namespace std;

struct RIFFHeader {
  char fourCC[4];
  int32_t fileSize;
  char fileType[4];
};

struct AviFrame {
  char fourCC[4];
  int32_t chunkSize;
  char *frameData;
  bool dirty;
};

class AviFrameWrapper : public Frame {
  private:
    AviFrame *f;
  public:
    AviFrameWrapper(AviFrame *f): f(f) {};
    virtual long getFrameSize() {
      return f->chunkSize;
    };
    virtual char *getFrameData(int n=0, int com=0) {
      return f->frameData;
    };
    virtual bool isDirty() {
      return f->dirty;
    };
    virtual void setDirty() {
      f->dirty = 1;
    };
};

struct AviList {
  char list[4];
  int32_t listSize;
  char fourCC[4];
};

struct AviHeader {
  char header[4];
  int32_t cb;
  int32_t microSecPerFrame;
  int32_t maxBytesPerSec;
  int32_t paddingGranularity;
  int32_t flags;
  int32_t totalFrames;
  int32_t initialFrames;
  int32_t streams;
  int32_t suggestedBufferSize;
  int32_t width;
  int32_t height;
  int32_t reserved[4];
};

struct AviStreamHeader {
  char fourCC[4];
  int32_t cb;
  char fccType[4];
  char fccHandler[4];
  int32_t flags;
  int16_t priority;
  int16_t language;
  int32_t initialFrames;
  int32_t scale;
  int32_t rate;
  int32_t start;
  int32_t length;
  int32_t suggestedBufferSize;
  int32_t quality;
  int32_t sampleSize;
  struct {
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
  } rcFrame;
};

struct BitmapInfoHeader {
  char fourCC[4];
  uint32_t size;
  uint32_t sizeAgain; // No idea why this is here?
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bitCount;
  uint32_t compression;
  uint32_t sizeImage;
  int32_t xPelsPerMeter;
  int32_t yPelsPerMeter;
  uint32_t clrUsed;
  uint32_t clrImportant; 
};

struct JunkChunk {
  char fourCC[4];
  uint32_t size;
};

struct WaveFormatEX {
  char fourCC[4];
  uint32_t padding; // Don't know why this is here
  uint16_t wFormatTag;
  uint16_t nChannels;
  uint32_t nSamplesPerSec;
  uint32_t nAvgBytesPerSec;
  uint16_t nBlockAlign;
  uint16_t wBitsPerSample;
};

class AVIDecoder : public VideoDecoder {
  private:
    string filePath;
    FILE *f;
    // Lock around filehandler access
    mutex mtx; 

    struct RIFFHeader riffHeader;
    struct AviHeader aviHeader;
    struct BitmapInfoHeader bitmapInfoHeader;
    struct AviFrame *frameChunks;

    long chunksOffset;
    int nextFrame = 1;
    int nextOffset = 0;
    char capacity = 100;
    bool hidden = false;

  public:
    AVIDecoder(string filePath): filePath(filePath) {
      int read = 0;
      this->f = fopen(this->filePath.c_str(), "rb+"); 
      fseek(f, 0, SEEK_SET);
 
      read = fread(&this->riffHeader, 4, 3, f);
      if (strncmp(this->riffHeader.fourCC, "RIFF", 4) != 0) {
        printf("File is not an AVI file\n");
        exit(0);
      }

      printf("Riff?: %.4s\n", this->riffHeader.fourCC);
      printf("filesize: %d\n", this->riffHeader.fileSize);
      printf("fileType: %s\n", this->riffHeader.fileType);

      struct AviList aviList;
      read = fread(&aviList, 4, 3, f);

      read = fread(&this->aviHeader, 1, sizeof(AviHeader), f);
      printf("microsecperframe: %d\n", this->aviHeader.microSecPerFrame); 
      printf("totalframes: %d\n", this->aviHeader.totalFrames); 
      printf("width: %d\n", this->aviHeader.width); 
      printf("height: %d\n", this->aviHeader.height); 
      printf("streams: %d\n", this->aviHeader.streams);

      struct AviList aviStreamList;
      read = fread(&aviStreamList, 4, 3, f);

      struct AviStreamHeader videoStreamHeader;
      read = fread(&videoStreamHeader, 1, sizeof(AviStreamHeader), f);
      read = fread(&this->bitmapInfoHeader, 1, sizeof(BitmapInfoHeader), f); 
      printf("fcchandler: %.4s\n", videoStreamHeader.fccHandler);
      printf("compression: %d\n", this->bitmapInfoHeader.compression);
     
      this->readJunk();

      read = fread(&aviStreamList, 4, 3, f);

      struct AviStreamHeader audioStreamHeader;
      read = fread(&audioStreamHeader, 1, sizeof(AviStreamHeader), f);

      struct WaveFormatEX audioInfoHeader;
      read = fread(&audioInfoHeader, 1, sizeof(WaveFormatEX), f);

      this->readJunk();

      read = fread(&aviList, 1, sizeof(AviList), f);
      fseek(f, aviList.listSize - 4, SEEK_CUR);

      this->readJunk();

      // Now we are at the start of the movi chunks
      read = fread(&aviList, 1, sizeof(AviList), f);
      printf("aviList: %.4s\n", aviList.fourCC);
      printf("chunkOffset: %lu\n", ftell(f));
      this->chunksOffset = ftell(f);

      this->frameChunks = (AviFrame *)malloc(sizeof(AviFrame) * this->aviHeader.totalFrames);
     
      // Now we can start reading chunks
      int i = 0;
      struct AviFrame tempChunk;
      printf("Reading AVI chunks...\n");
      while (i < this->aviHeader.totalFrames) {
        loadBar(i, this->aviHeader.totalFrames - 1, 50);
        read = fread(&tempChunk, 1, 4 + 4, f);
        //printf("\e[1A"); 
        //printf("\e[0K\rReading chunk '%d', type: %.4s, size: %d\n", i, tempChunk.fourCC, tempChunk.chunkSize);
        if (strncmp(tempChunk.fourCC, "00db", 4) == 0) {
          fseek(f, -8, SEEK_CUR);
          read = fread(&frameChunks[i], 1, 4 + 4, f);
          frameChunks[i].frameData = (char *)malloc(frameChunks[i].chunkSize);
          read = fread(frameChunks[i].frameData, frameChunks[i].chunkSize, 1, f);
          frameChunks[i].dirty = 0;
          i ++;
        } else {
          fseek(f, tempChunk.chunkSize, SEEK_CUR);
        }
      }
      printf("Finished parsing AVI file\n");
    };
    void readJunk() {
      struct JunkChunk junk;
      int read = fread(&junk, 1, sizeof(JunkChunk), f);
      if (strncmp(junk.fourCC, "JUNK", 4) == 0) {
        fseek(f, junk.size, SEEK_CUR);
      } else {
        fseek(f, -sizeof(JunkChunk), SEEK_CUR);
      }
    };
    virtual ~AVIDecoder() {
      this->writeBack();
      free(this->frameChunks);
      fclose(this->f);
    };
    virtual void writeBack() {
      // Lock needed for the case in which flush is called twice in quick sucsession
      mtx.lock();
      fseek(f, this->chunksOffset, SEEK_SET); 
      int i = 0;
      char fourCC[4]; 
      int32_t chunkSize; 
      printf("Writing back to disc...\n");
      while (i < this->aviHeader.totalFrames) {
        loadBar(i, this->aviHeader.totalFrames - 1, 50);
        int read = fread(&fourCC, 1, 4, f);
        if (read != 4) {
          // Error reading file, unlock and return
          mtx.unlock();
          return;
        }
        if (strncmp(fourCC, "00db", 4) == 0) {
          fseek(f, -4, SEEK_CUR);
          fwrite(&this->frameChunks[i], 1, 8, f);
          if (this->frameChunks[i].dirty) {
            //printf("\e[1A"); 
            //printf("\e[0K\rwriting chunk %d, size: %d\n", i, this->frameChunks[i].chunkSize);
            fwrite(this->frameChunks[i].frameData, this->frameChunks[i].chunkSize, 1, f);
            // This frame is no longer dirty
            this->frameChunks[i].dirty = 0;
          } else {
            // No need to write if nothing has changed in this frame
            fseek(f, this->frameChunks[i].chunkSize, SEEK_CUR);
          }
          i ++;
        } else {
          int read = fread(&chunkSize, 1, 4, f);
          fseek(f, chunkSize, SEEK_CUR);
        }
      }
      mtx.unlock();
    };
    virtual Frame *getFrame(int frame) {
      return new AviFrameWrapper(&this->frameChunks[frame]); 
    };                                     
    virtual Frame *getHeaderFrame() {
      if (this->hidden) {
        return new AviFrameWrapper(&this->frameChunks[this->getNumberOfFrames()/2]);
      } else {
        return new AviFrameWrapper(&this->frameChunks[0]);
      }
    };
    virtual int getFileSize() {
      return this->riffHeader.fileSize; 
    };                                                 
    virtual int getNumberOfFrames() {
      return this->aviHeader.totalFrames; 
    };
    virtual void getNextFrameOffset(int *frame, int *offset) {
      *frame = this->nextFrame;
      *offset = this->nextOffset;
    };
    virtual void setNextFrameOffset(int frame, int offset) {
       this->nextFrame = frame;
       this->nextOffset = offset;
    };
    virtual int getFrameHeight() {
      return this->aviHeader.height;  
    };
    virtual int getFrameWidth() {
      return this->aviHeader.width; 
    };
    virtual void setCapacity(char capacity) { 
      this->capacity = capacity;
    };
    virtual void setHiddenVolume() {
      this->hidden = true;  
    };
    virtual int getFrameSize() {
      // 24 bits per pixel
      return (int)floor(this->aviHeader.width * this->aviHeader.height * 3 * (capacity / 100.0));
    };
};
