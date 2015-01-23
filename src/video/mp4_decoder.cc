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
      AVFormatContext *pFormatCtx = NULL;
      AVCodecContext  *pCodecCtx = NULL;
      AVCodec         *pCodec = NULL;
      AVFrame         *pFrame = NULL; 
      AVDictionary    *optionsDict = NULL;
      AVPacket        packet;
      int             frameFinished;
      int             videoStream = 0;

      // Register all formats and codecs
      av_register_all();

      // Open video file
      if (avformat_open_input(&pFormatCtx, filePath.c_str(), NULL, NULL) != 0) {
        printf("FFmpeg failed to open file.\n");
        exit(0);
      }

      // Retrieve stream information
      if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
        printf("FFmpeg faild to find stream info.\n");
        exit(0);
      }

      // Dump information about file onto standard error
      av_dump_format(pFormatCtx, 0, filePath.c_str(), false);
      videoStream = -1;
      for (int i = 0; i < pFormatCtx->nb_streams; i ++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
          videoStream = i;
          break;
        }
      }
      if (videoStream == -1) {
        printf("Couldn't find a video stream");
        exit(0);
      }
        
      // Get a pointer to the codec context for the video stream
      pCodecCtx = pFormatCtx->streams[videoStream]->codec;
        
      // Find the decoder for the video stream
      pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
      if (pCodec == NULL) {
        printf("Unsupported codec!\n");
        exit(0);
      }

      if(avcodec_open2(pCodecCtx, pCodec, &optionsDict) < 0) {
        printf("Couldn't open codec");
        exit(0);
      }

      // Allocate video frame
      pFrame=av_frame_alloc();
      while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
          // Decode video frame
          avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

          if(frameFinished) {  
            ///printf("Got a frame\n");
            if (pFrame->pict_type != AV_PICTURE_TYPE_I) {
              printMVMatrix(0, pFrame, pCodecCtx);
            }
           /* if (pFrame->pict_type != AV_PICTURE_TYPE_I && pFrame->motion_val) {
              printf("Motion vector?: %d\n", pFrame->motion_val[0][0][0]);
            }*/
          }
        }
      }
    };
void print_vector(int x, int y, int dx, int dy)
{
  if (dx != 10000 && dy != 10000){
    //sum = sum + sqrt((double)dx*(double)dx/w/w + (double)dy*(double)dy/h/h);
    //count++;
  }
    printf("%d %d ; %d %d\n", x, y, dx, dy);
}

    void printMVMatrix(int index, AVFrame *pict, AVCodecContext *ctx)
{
    printf("hello");
    const int mb_width  = (ctx->width + 15) / 16;
    const int mb_height = (ctx->height + 15) / 16;
    const int mb_stride = mb_width + 1;
    const int mv_sample_log2 = 4 - pict->motion_subsample_log2;
    const int mv_stride = (mb_width << mv_sample_log2) + (ctx->codec_id == CODEC_ID_H264 ? 0 : 1);
    const int quarter_sample = (ctx->flags & CODEC_FLAG_QPEL) != 0;
    const int shift = 1 + quarter_sample;

    printf("Getting hre");

    //printf("frame %d, %d x %d\n", index, mb_height, mb_width);

    for (int mb_y = 0; mb_y < mb_height; mb_y++) {
  for (int mb_x = 0; mb_x < mb_width; mb_x++) {
      const int mb_index = mb_x + mb_y * mb_stride;
      if (pict->motion_val) {
    for (int type = 0; type < 3; type++) {
        int direction = 0;
        switch (type) {
      case 0:
          if (pict->pict_type != AV_PICTURE_TYPE_P)
        continue;
          direction = 0;
          break;
      case 1:
          if (pict->pict_type != AV_PICTURE_TYPE_B)
              continue;
          direction = 0;
          break;
      case 2:
          if (pict->pict_type != AV_PICTURE_TYPE_B)
        continue;
          direction = 1;
          break;
        }

      printf("mb_index: %d\n", mb_index);

        if (!USES_LIST(pict->mb_type[mb_index], direction)) {
#define NO_MV 10000
      if (IS_8X8(pict->mb_type[mb_index])) {
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
      } else if (IS_16X8(pict->mb_type[mb_index])) {
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
      } else if (IS_8X16(pict->mb_type[mb_index])) {
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
      } else {
          print_vector(mb_x, mb_y, NO_MV, NO_MV);
      }
#undef NO_MV
      continue;
        }

        if (IS_8X8(pict->mb_type[mb_index])) {
      for (int i = 0; i < 4; i++) {
          int xy = (mb_x*2 + (i&1) + (mb_y*2 + (i>>1))*mv_stride) << (mv_sample_log2-1);
          int dx = (pict->motion_val[direction][xy][0]>>shift);
          int dy = (pict->motion_val[direction][xy][1]>>shift);
          print_vector(mb_x, mb_y, dx, dy);
      }
        } else if (IS_16X8(pict->mb_type[mb_index])) {
      for (int i = 0; i < 2; i++) {
          int xy = (mb_x*2 + (mb_y*2 + i)*mv_stride) << (mv_sample_log2-1);
          int dx = (pict->motion_val[direction][xy][0]>>shift);
          int dy = (pict->motion_val[direction][xy][1]>>shift);

          if (IS_INTERLACED(pict->mb_type[mb_index]))
        dy *= 2;

          print_vector(mb_x, mb_y, dx, dy);
      }
        } else if (IS_8X16(pict->mb_type[mb_index])) {
      for (int i = 0; i < 2; i++) {
          int xy =  (mb_x*2 + i + mb_y*2*mv_stride) << (mv_sample_log2-1);
          int dx = (pict->motion_val[direction][xy][0]>>shift);
          int dy = (pict->motion_val[direction][xy][1]>>shift);

          if (IS_INTERLACED(pict->mb_type[mb_index]))
        dy *= 2;

          print_vector(mb_x, mb_y, dx, dy);
      }
        } else {
      int xy = (mb_x + mb_y*mv_stride) << mv_sample_log2;
      int dx = (pict->motion_val[direction][xy][0]>>shift);
      int dy = (pict->motion_val[direction][xy][1]>>shift);
      print_vector(mb_x, mb_y, dx, dy);
        }
    }
      }
      //printf("--\n");
  }
  //printf("====\n");
    }
}
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
        return this->getFrame(this->getNumberOfFrames() / 2);
      } else {
        return this->getFrame(0);
      }
    };
    virtual int getFileSize() {
      return 100;
    };                                                 
    virtual int getNumberOfFrames() {
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
    virtual int getFrameHeight() {
      return this->height; 
    };
    virtual int getFrameWidth() {
      return this->width; 
    };
    virtual void setCapacity(char capacity) { 
      this->capacity = capacity;
    };
    virtual void setHiddenVolume() {
      this->hidden = true;
    };
    virtual int getFrameSize() {
      return (int)floor(this->width * this->height * 63 * (capacity / 100.0) * 2);
    };
};
