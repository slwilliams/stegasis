#include <stdio.h>
#include <string.h>
#include <string>

#include "video_decoder.h"

using namespace std;

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
    struct WaveFormatEX audioInfoHeader;
    struct AviChunk *frameChunks;
    long chunksOffset;
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
     printf("audio header ff: %.4s\n", this->audioStreamHeader.fourCC);
     printf("audio fcctype: %.4s\n", this->audioStreamHeader.fccType);
     printf("audio cb: %d\n", this->audioStreamHeader.cb);

     fread(&this->audioInfoHeader, 1, sizeof(WaveFormatEX), f);
     printf("wavefourcc: %.4s\n", this->audioInfoHeader.fourCC);
     printf("audio wFormatTag: %d\n", this->audioInfoHeader.wFormatTag);
     printf("audio channels: %d\n", this->audioInfoHeader.nChannels);
     printf("audio sample rate: %d\n", this->audioInfoHeader.nSamplesPerSec);
     printf("audio bitrate: %d\n", this->audioInfoHeader.wBitsPerSample);

     fread(&junk, 1, sizeof(JunkChunk), f);
     fseek(f, junk.size, SEEK_CUR);
     
     fread(&this->aviList, 1, sizeof(AviList), f);
     fseek(f, this->aviList.listSize - 4, SEEK_CUR);

     fread(&junk, 1, sizeof(JunkChunk), f);
     printf("junk?: %.4s\n", junk.fourCC);
     fseek(f, junk.size, SEEK_CUR);

     // Now we are at the start of the movi chunks

     fread(&this->aviList, 1, sizeof(AviList), f);
     this->chunksOffset = ftell(f);
     printf("list type: %.4s\n", this->aviList.fourCC);
     printf("list size: %d\n", this->aviList.listSize);

     this->frameChunks = (AviChunk *)malloc(sizeof(AviChunk) * this->aviHeader.totalFrames);
     
     //now we can start reading chunks
     int i = 0;
     AviChunk tempChunk;
     while (i < this->aviHeader.totalFrames) {
       fread(&tempChunk, 1, 4 + 4, f);
       printf("chunk type: %.4s\n", tempChunk.fourCC);
       printf("chunk size: %d\n", tempChunk.chunkSize);
       if (strncmp(tempChunk.fourCC, "00db", 4) == 0) {
         fseek(f, -8, SEEK_CUR);
         fread(&frameChunks[i], 1, 4 + 4, f);
         char *frameData = (char *)malloc(frameChunks[i].chunkSize);
         frameChunks[i].frameData = frameData;
         i ++;
       }
       fseek(f, tempChunk.chunkSize, SEEK_CUR);
     }
    };
    ~AVIDecoder() {
      // Write back the frame data
      fseek(f, this->chunksOffset, SEEK_SET); 
      int i = 0;
      char fourCC[4]; 
      int32_t chunkSize; 
      while (i < this->aviHeader.totalFrames) {
        fread(&fourCC, 1, 4, f);
        if (strncmp(fourCC, "00db", 4) == 0) {
          fseek(f, -4, SEEK_CUR);
          fwrite(&this->frameChunks[i], 1, 8, f);
          fwrite(&this->frameChunks[i].frameData, 1, this->frameChunks[i].chunkSize, f);
          i ++;
        } else {
          fread(&chunkSize, 1, 4, f);
          fseek(f, chunkSize, SEEK_CUR);
        }
      }
      fclose(this->f);
    };
    virtual AviChunk getFrame(int frame) {
      return this->frameChunks[frame];
    };                                     
    virtual void writeFrame(int frame, char frameData[]) {
      memcpy(this->frameChunks[frame].frameData, &frameData, this->frameChunks[frame].chunkSize);
    };                   
    virtual int getFileSize() {
      return this->riffHeader.fileSize; 
    };                                                 
    virtual int numberOfFrames() {
      return this->aviHeader.totalFrames; 
    };   
};
