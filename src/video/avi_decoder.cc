#include <stdio.h>
#include <string.h>
#include <string>

#include "video_decoder.h"

using namespace std;

class AVIDecoder : public VideoDecoder {
  private:
    string filePath;
    FILE *f;

    struct RIFFHeader riffHeader;
    struct AviHeader aviHeader;
    struct BitmapInfoHeader bitmapInfoHeader;
    struct AviChunk *frameChunks;

    long chunksOffset;

  public:
    AVIDecoder(string filePath): filePath(filePath) {
      this->f = fopen(this->filePath.c_str(), "rb+"); 
      fseek(f, 0, SEEK_SET);
 
      fread(&this->riffHeader, 4, 3, f);
      printf("riff?: %.4s\n", this->riffHeader.fourCC);
      printf("filesize: %d\n", this->riffHeader.fileSize);
      printf("fileTYpe: %s\n", this->riffHeader.fileType);

      struct AviList aviList;
      fread(&aviList, 4, 3, f);
      printf("lisesize: %d\n", aviList.listSize);

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
      printf("fcctype: %.4s\n", videoStreamHeader.fccType);
      printf("fcchandler: %.4s\n", videoStreamHeader.fccHandler);
    
      fread(&this->bitmapInfoHeader, 1, sizeof(BitmapInfoHeader), f); 
      printf("bitmap fourCC: %.4s\n", this->bitmapInfoHeader.fourCC);
      printf("size: %d\n", this->bitmapInfoHeader.size);
      printf("bitmap witdh: %d\n", this->bitmapInfoHeader.width);
      printf("bitmap height: %d\n", this->bitmapInfoHeader.height);
      printf("planes: %d\n", this->bitmapInfoHeader.planes);
      printf("bitcount: %d\n", this->bitmapInfoHeader.bitCount);
      printf("compression: %d\n", this->bitmapInfoHeader.compression); 
     
      struct JunkChunk junk;
      fread(&junk, 1, sizeof(JunkChunk), f);
      fseek(f, junk.size, SEEK_CUR);

      fread(&aviStreamList, 4, 3, f);

      struct AviStreamHeader audioStreamHeader;
      fread(&audioStreamHeader, 1, sizeof(AviStreamHeader), f);
      printf("audio header ff: %.4s\n", audioStreamHeader.fourCC);
      printf("audio fcctype: %.4s\n", audioStreamHeader.fccType);
      printf("audio cb: %d\n", audioStreamHeader.cb);

      struct WaveFormatEX audioInfoHeader;
      fread(&audioInfoHeader, 1, sizeof(WaveFormatEX), f);
      printf("wavefourcc: %.4s\n", audioInfoHeader.fourCC);
      printf("audio wFormatTag: %d\n", audioInfoHeader.wFormatTag);
      printf("audio channels: %d\n", audioInfoHeader.nChannels);
      printf("audio sample rate: %d\n", audioInfoHeader.nSamplesPerSec);
      printf("audio bitrate: %d\n", audioInfoHeader.wBitsPerSample);

      fread(&junk, 1, sizeof(JunkChunk), f);
      fseek(f, junk.size, SEEK_CUR);
     
      fread(&aviList, 1, sizeof(AviList), f);
      fseek(f, aviList.listSize - 4, SEEK_CUR);

      fread(&junk, 1, sizeof(JunkChunk), f);
      fseek(f, junk.size, SEEK_CUR);

      // Now we are at the start of the movi chunks

      fread(&aviList, 1, sizeof(AviList), f);
      this->chunksOffset = ftell(f);
      printf("list type: %.4s\n", aviList.fourCC);
      printf("list size: %d\n", aviList.listSize);

      this->frameChunks = (AviChunk *)malloc(sizeof(AviChunk) * this->aviHeader.totalFrames);
     
      //now we can start reading chunks
      int i = 0;
      struct AviChunk tempChunk;
      while (i < this->aviHeader.totalFrames) {
        fread(&tempChunk, 1, 4 + 4, f);
        printf("chunk type: %.4s\n", tempChunk.fourCC);
        printf("chunk size: %d\n", tempChunk.chunkSize);
        if (strncmp(tempChunk.fourCC, "00db", 4) == 0) {
          fseek(f, -8, SEEK_CUR);
          fread(&frameChunks[i], 1, 4 + 4, f);
          char *frameData = (char *)malloc(frameChunks[i].chunkSize);
          fread(frameData, frameChunks[i].chunkSize, 1, f);
          frameChunks[i].frameData = frameData;
          i ++;
        } else {
          fseek(f, tempChunk.chunkSize, SEEK_CUR);
        }
      }
    };
    virtual ~AVIDecoder() {
      // Write back the frame data
      fseek(f, this->chunksOffset, SEEK_SET); 
      int i = 0;
      char fourCC[4]; 
      int32_t chunkSize; 
      while (i < this->aviHeader.totalFrames) {
        fread(&fourCC, 1, 4, f);
        printf("fourcc: %.4s\n", fourCC);
        if (strncmp(fourCC, "00db", 4) == 0) {
          fseek(f, -4, SEEK_CUR);
          fwrite(&this->frameChunks[i], 1, 8, f);
          printf("writing chunk %d, size: %d\n", i, this->frameChunks[i].chunkSize);
          int out = fwrite(this->frameChunks[i].frameData, this->frameChunks[i].chunkSize, 1, f);
          i ++;
        } else {
          fread(&chunkSize, 1, 4, f);
          fseek(f, chunkSize, SEEK_CUR);
        }
      }
      fclose(this->f);
    };
    virtual struct AviChunk getFrame(int frame) {
      return this->frameChunks[frame];
    };                                     
    virtual int getFileSize() {
      return this->riffHeader.fileSize; 
    };                                                 
    virtual int numberOfFrames() {
      return this->aviHeader.totalFrames; 
    };   
};
