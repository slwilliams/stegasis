#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <string>
#include <list>
#include <mutex> 
#include <math.h>
#include <sys/stat.h>

extern "C" {
  #include <ffmpeg/libavcodec/avcodec.h>
  #include <ffmpeg/libavformat/avformat.h>
  #include <ffmpeg/libavutil/motion_vector.h>
}
#include "common/progress_bar.h"
#include "video_decoder.h"

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

static int open_codec_context(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type) {
  int ret;
  AVStream *st;
  AVCodecContext *dec_ctx = NULL;
  AVCodec *dec = NULL;
  AVDictionary *opts = NULL;

  ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
  if (ret < 0) {
    fprintf(stderr, "Could not find %s stream in input\n", av_get_media_type_string(type));
    return ret;
  } else {
    *stream_idx = ret;
    st = fmt_ctx->streams[*stream_idx];

    /* find decoder for the stream */
    dec_ctx = st->codec;
    dec = avcodec_find_decoder(dec_ctx->codec_id);
    if (!dec) {
      fprintf(stderr, "Failed to find %s codec\n", av_get_media_type_string(type));
      return AVERROR(EINVAL);
    }

    /* Init the video decoder */
    av_dict_set(&opts, "flags2", "+export_mvs", 0);
    if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
      fprintf(stderr, "Failed to open %s codec\n", av_get_media_type_string(type));
      return ret;
    }
  }

  return 0;
}

static int decode_packet(AVFormatContext *fmt, AVPacket pkt, AVFrame *frame, AVCodecContext *video_dec_ctx, int video_stream_idx, int *got_frame, int cached) {
  int decoded = pkt.size;

  *got_frame = 0;

  if (pkt.stream_index == video_stream_idx) {
      int ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
      if (ret < 0) {
          fprintf(stderr, "Error decoding video frame\n");
          return ret;
      }

      if (*got_frame) {
          int i;
          if (frame->motion_val[0]) {
            printf("yay?\n");
          }
          AVFrameSideData *sd;

          sd = av_frame_get_side_data(frame, AV_FRAME_DATA_MOTION_VECTORS);
          if (sd) {
              AVMotionVector *mvs = (AVMotionVector *)sd->data;
              for (i = 0; i < sd->size / sizeof(*mvs); i++) {
                  AVMotionVector *mv = &mvs[i];
                  /*printf("%d,%2d,%2d,%2d,%4d,%4d,%4d,%4d,0x%" PRIx64"\n",
                         0, mv->source,
                         mv->w, mv->h, mv->src_x, mv->src_y,
                         mv->dst_x, mv->dst_y, mv->flags);*/
                  mv->dst_x=20;
                  mv->dst_y=20;
              }
          }
      }
  }

  return decoded;
}



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
      AVFormatContext *fmt_ctx = NULL;                                         
      AVCodecContext *video_dec_ctx = NULL;                                    
      AVStream *video_stream = NULL;                                           
      const char *src_filename = NULL;                                         

      int video_stream_idx = -1;                                               
      AVFrame *frame = NULL;                                                   
      AVPacket pkt;                                                            
      int video_frame_count = 0;  

      // Register all formats and codecs
      av_register_all();

      // Open video file
      if (avformat_open_input(&fmt_ctx, filePath.c_str(), NULL, NULL) != 0) {
        printf("FFmpeg failed to open file.\n");
        exit(0);
      }

      // Retrieve stream information
      if (avformat_find_stream_info(fmt_ctx, NULL) < 0){
        printf("FFmpeg faild to find stream info.\n");
        exit(0);
      }

      if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];
        video_dec_ctx = video_stream->codec;
        video_dec_ctx->debug_mv = 1;
        video_dec_ctx->flags2 |= CODEC_FLAG2_EXPORT_MVS;
      }

      // Dump information about file onto standard error
      av_dump_format(fmt_ctx, 0, filePath.c_str(), false);
      
      if (!video_stream) {
        printf("Could not fine video stream.\n");
        exit(0);
      }

      frame = av_frame_alloc();

      av_init_packet(&pkt);
      pkt.data = NULL;
      pkt.size = 0;

      int got_frame = 0;
      
      /* read frames from the file */
      while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;
        do {
          int ret = decode_packet(fmt_ctx, pkt, frame, video_dec_ctx, video_stream_idx, &got_frame, 0);
          if (ret < 0) {
            break;
          }
          pkt.data += ret;
          pkt.size -= ret;
        } while (pkt.size > 0);
        av_free_packet(&orig_pkt);
      }

      /* flush cached frames */
      pkt.data = NULL;
      pkt.size = 0;
      do {
        decode_packet(fmt_ctx, pkt, frame, video_dec_ctx, video_stream_idx, &got_frame, 1);
      } while (got_frame);

      avcodec_close(video_dec_ctx);
      avformat_close_input(&fmt_ctx);
      av_frame_free(&frame);
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
