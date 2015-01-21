#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <string>
#include <list>
#include <mutex> 
#include <math.h>
#include <sys/stat.h>

extern "C"{

#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavformat/avformat.h>
}
#include "common/progress_bar.h"
#include "video_decoder.h"

#define IS_INTERLACED(a) ((a)&MB_TYPE_INTERLACED)
#define IS_16X16(a)      ((a)&MB_TYPE_16x16)
#define IS_16X8(a)       ((a)&MB_TYPE_16x8)
#define IS_8X16(a)       ((a)&MB_TYPE_8x16)
#define IS_8X8(a)        ((a)&MB_TYPE_8x8)
#define USES_LIST(a, list) ((a) & ((MB_TYPE_P0L0|MB_TYPE_P1L0)<<(2*(list))))

using namespace std;

struct MP4Chunk {
  bool dirty;
  int frame;
};

class MP4ChunkWrapper : public Chunk {
  private:
    MP4Chunk *c;
  public:
    MP4ChunkWrapper(MP4Chunk *c): c(c) {};
    virtual long getChunkSize() {
      return this->chunkSize;
    };
    virtual char *getFrameData(int n, int c) {
      return NULL;
    };
    virtual bool isDirty() {
      return this->c->dirty;
    };
    virtual void setDirty() {
      this->c->dirty = 1;
    };
};

class MP4Decoder : public VideoDecoder {
  private:
    string filePath;
    mutex mtx; 

    bool hidden = false;

    int nextFrame = 1;
    int nextOffset = 0;
    char capacity = 100;
    int totalFrames;
    int fileSize;
    int width;
    int height;
    int fps;

  public:
    MP4Decoder(string filePath): filePath(filePath) {
      this->fps = 25; 
      printf("fps: %d\n", this->fps);
      AVFormatContext *pFormatCtx = NULL;
      AVCodecContext  *pCodecCtx;
      AVCodec         *pCodec;
      AVFrame         *pFrame; 
      int             videoStream;

      //save height and width
      double h = 1280;
      double w = 720;

      // Register all formats and codecs
      av_register_all();

      // Open video file
      if (avformat_open_input(&pFormatCtx, (const char *)filePath.c_str(), NULL, NULL) != 0) {
        printf("BAD!!\n");
        exit(0);
      }
      //
      // Retrieve stream information
      if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
        printf("Also bas\n");
        exit(0);
      }

      // Dump information about file onto standard error
      av_dump_format(pFormatCtx, 0, filePath.c_str(), false);
    };
    virtual ~MP4Decoder() {
      this->writeBack();
    };
    virtual void writeBack() {
      // Lock needed for the case in which flush is called twice in quick sucsession
      mtx.lock();
      mtx.unlock();
    };
    virtual Chunk *getFrame(int frame) {
      mtx.lock();
      mtx.unlock();
    };                                     
    virtual Chunk *getHeaderFrame() {
      if (this->hidden) {
        return this->getFrame(this->numberOfFrames() / 2);
      } else {
        return this->getFrame(0);
      }
    };
    virtual int getFileSize() {
      return 100;
    };                                                 
    virtual int numberOfFrames() {
      return this->totalFrames; 
    };
    virtual void getNextFrameOffset(int *frame, int *offset) {
      *frame = this->nextFrame;
      *offset = this->nextOffset;
    };
    virtual void setNextFrameOffset(int frame, int offset) {
      this->nextFrame = frame;
      this->nextOffset = offset;
    };
    virtual int frameHeight() {
      return this->height; 
    };
    virtual int frameWidth() {
      return this->width; 
    };
    virtual void setCapacity(char capacity) { 
      this->capacity = capacity;
    };
    virtual void setHiddenVolume() {
      this->hidden = true;
    };
    virtual int frameSize() {
      return (int)floor(this->width * this->height * 63 * (capacity / 100.0) * 2);
    };
};
