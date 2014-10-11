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
  char *data;
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

class AVIDecoder : public VideoDecoder {
  private:
    string filePath;
    struct RIFFHeader riffHeader;
    struct AviList aviList;
    struct AviHeader aviHeader;
    int32_t fileSize;
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
    };
    ~AVIDecoder() {
      fclose(this->f);
    };
    virtual char **getFrame(int frame) {
      
    };                                     
    virtual bool writeFrame(int frame, char frameData[]) {
    
    };                   
    virtual int getFileSize() {
     
    };                                                 
    virtual int numberOfFrames() {
    
    };   
};
