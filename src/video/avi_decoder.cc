#include <stdio.h>
#include <string>

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
  char *data;
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

class AVIDecoder : public VideoDecoder {
  private:
    string filePath;
    struct RIFFHeader riffHeader;
    struct AviList aviList;
    struct AviList aviStreamList;
    struct AviStreamHeader videoStreamHeader;
    struct AviStreamHeader audioStreamHeader;
    struct AviHeader aviHeader;
    struct BitmapInfoHeader bitmapInfoHeader;
    FILE *f;
  public:
    AVIDecoder(string filePath): filePath(filePath) {
     this->f = fopen(this->filePath.c_str(), "rb+"); 

     fseek(f, 0, SEEK_SET);
     fread(&this->riffHeader, 4, 3, f);
     printf("riff?: %.4s\n", this->riffHeader.fourCC);
     printf("filesize: %d\n", this->riffHeader.fileSize);
     printf("fileTYpe: %s\n", this->riffHeader.fileType);

     fread(&this->aviList, 4, 3, f);
     printf("lisesize: %d\n", this->aviList.listSize);

     fread(&this->aviHeader, 1, sizeof(AviHeader), f);
     printf("microsecperframe: %d\n", this->aviHeader.microSecPerFrame); 
     printf("totalframes: %d\n", this->aviHeader.totalFrames); 
     printf("width: %d\n", this->aviHeader.width); 
     printf("height: %d\n", this->aviHeader.height); 
     printf("streams: %d\n", this->aviHeader.streams);

     fread(&this->aviStreamList, 4, 3, f);
     fread(&this->videoStreamHeader, 1, sizeof(AviStreamHeader), f);
     printf("fcctype: %.4s\n", this->videoStreamHeader.fccType);
     printf("fcchandler: %.4s\n", this->videoStreamHeader.fccHandler);
     //Note DIB -> device independant bitmap i.e. uncompressed, what we want
    
     fread(&this->bitmapInfoHeader, 1, sizeof(BitmapInfoHeader), f); 
     printf("bitmap fourCC: %.4s\n", this->bitmapInfoHeader.fourCC);
     printf("size: %d\n", this->bitmapInfoHeader.size);
     printf("bitmap witdh: %d\n", this->bitmapInfoHeader.width);
     printf("bitmap height: %d\n", this->bitmapInfoHeader.height);
     printf("planes: %d\n", this->bitmapInfoHeader.planes);
     printf("bitcount: %d\n", this->bitmapInfoHeader.bitCount);
     printf("compression: %d\n", this->bitmapInfoHeader.compression); 
     // 0 -> uncompressed
     
     JunkChunk junk;
     fread(&junk, 1, sizeof(JunkChunk), f);
     fseek(f, junk.size, SEEK_CUR);

     fread(&this->aviStreamList, 4, 3, f);
     fread(&this->audioStreamHeader, 1, sizeof(AviStreamHeader), f);
     printf("fcctype_audio: %.4s\n", this->audioStreamHeader.fccType);
     printf("fcchandler_audio: %.4s\n", this->audioStreamHeader.fccHandler);
    };
    ~AVIDecoder() {
      fclose(this->f);
    };
    virtual char **getFrame(int frame) {
      
    };                                     
    virtual bool writeFrame(int frame, char frameData[]) {
    
    };                   
    virtual int getFileSize() {
      return this->riffHeader.fileSize; 
    };                                                 
    virtual int numberOfFrames() {
      return this->aviHeader.totalFrames; 
    };   
};
