#include <stdio.h>
#include <string.h>
#include <string>
#include <mutex> 

#include "video_decoder.h"

using namespace std;

struct RIFFHeader {
  char fourCC[4];
  int32_t fileSize;
  char fileType[4];
};

struct AviChunk {
  char fourCC[4];
  int32_t chunkSize;
  char *frameData;
  bool dirty;
};

class AviChunkWrapper : public Chunk {
  private:
    AviChunk *c;
  public:
    AviChunkWrapper(AviChunk *c): c(c) {};
    virtual int getChunkSize() {
      return c->chunkSize;
    };
    virtual char *getFrameData() {
      return c->frameData;
    };
    virtual bool isDirty() {
      return c->dirty;
    };
    virtual void setDirty() {
      c->dirty = 1;
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
    struct AviChunk *frameChunks;

    long chunksOffset;
    int nextFrame = 1;
    int nextOffset = 0;

  public:
    AVIDecoder(string filePath): filePath(filePath) {
      this->f = fopen(this->filePath.c_str(), "rb+"); 
      fseek(f, 0, SEEK_SET);
 
      fread(&this->riffHeader, 4, 3, f);
      if (strncmp(this->riffHeader.fourCC, "RIFF", 4) != 0) {
        printf("File is not an AVI file\n");
        exit(0);
      }

      printf("riff?: %.4s\n", this->riffHeader.fourCC);
      printf("filesize: %d\n", this->riffHeader.fileSize);
      printf("fileTYpe: %s\n", this->riffHeader.fileType);

      struct AviList aviList;
      fread(&aviList, 4, 3, f);

      fread(&this->aviHeader, 1, sizeof(AviHeader), f);
      printf("microsecperframe: %d\n", this->aviHeader.microSecPerFrame); 
      printf("totalframes: %d\n", this->aviHeader.totalFrames); 
      printf("width: %d\n", this->aviHeader.width); 
      printf("height: %d\n", this->aviHeader.height); 
      printf("streams: %d\n", this->aviHeader.streams);

      struct AviList aviStreamList;
      fread(&aviStreamList, 4, 3, f);

      struct AviStreamHeader videoStreamHeader;
      fread(&videoStreamHeader, 1, sizeof(AviStreamHeader), f);
      fread(&this->bitmapInfoHeader, 1, sizeof(BitmapInfoHeader), f); 
     
      // TODO: Is this junk chunk in all AVI files?
      struct JunkChunk junk;
      fread(&junk, 1, sizeof(JunkChunk), f);
      fseek(f, junk.size, SEEK_CUR);

      fread(&aviStreamList, 4, 3, f);

      struct AviStreamHeader audioStreamHeader;
      fread(&audioStreamHeader, 1, sizeof(AviStreamHeader), f);

      struct WaveFormatEX audioInfoHeader;
      fread(&audioInfoHeader, 1, sizeof(WaveFormatEX), f);

      fread(&junk, 1, sizeof(JunkChunk), f);
      fseek(f, junk.size, SEEK_CUR);
     
      fread(&aviList, 1, sizeof(AviList), f);
      fseek(f, aviList.listSize - 4, SEEK_CUR);

      fread(&junk, 1, sizeof(JunkChunk), f);
      fseek(f, junk.size, SEEK_CUR);

      // Now we are at the start of the movi chunks

      fread(&aviList, 1, sizeof(AviList), f);
      this->chunksOffset = ftell(f);

      this->frameChunks = (AviChunk *)malloc(sizeof(AviChunk) * this->aviHeader.totalFrames);
     
      // Now we can start reading chunks
      int i = 0;
      struct AviChunk tempChunk;
      while (i < this->aviHeader.totalFrames) {
        fread(&tempChunk, 1, 4 + 4, f);
        printf("\e[1A"); 
        printf("\e[0K\rReading chunk '%d', type: %.4s, size: %d\n", i, tempChunk.fourCC, tempChunk.chunkSize);
        if (strncmp(tempChunk.fourCC, "00db", 4) == 0) {
          fseek(f, -8, SEEK_CUR);
          fread(&frameChunks[i], 1, 4 + 4, f);
          frameChunks[i].frameData = (char *)malloc(frameChunks[i].chunkSize);
          fread(frameChunks[i].frameData, frameChunks[i].chunkSize, 1, f);
          frameChunks[i].dirty = 0;
          i ++;
        } else {
          fseek(f, tempChunk.chunkSize, SEEK_CUR);
        }
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
      while (i < this->aviHeader.totalFrames) {
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
            printf("\e[1A"); 
            printf("\e[0K\rwriting chunk %d, size: %d\n", i, this->frameChunks[i].chunkSize);
            fwrite(this->frameChunks[i].frameData, this->frameChunks[i].chunkSize, 1, f);
            // This frame is no longer dirty
            this->frameChunks[i].dirty = 0;
          } else {
            // No need to write if nothing has changed in this frame
            fseek(f, this->frameChunks[i].chunkSize, SEEK_CUR);
          }
          i ++;
        } else {
          fread(&chunkSize, 1, 4, f);
          fseek(f, chunkSize, SEEK_CUR);
        }
      }
      mtx.unlock();
    };
    virtual Chunk *getFrame(int frame) {
      return new AviChunkWrapper(&this->frameChunks[frame]); 
    };                                     
    virtual int getFileSize() {
      return this->riffHeader.fileSize; 
    };                                                 
    virtual int numberOfFrames() {
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
   virtual int frameSize() {
     // 24 bits per pixel
     return this->aviHeader.width * this->aviHeader.height * 3;
   };
};
