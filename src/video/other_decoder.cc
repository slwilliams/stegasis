#include <stdio.h>
#include <string.h>
#include <string>
#include <mutex> 
#include <math.h>

#include "common/progress_bar.h"
#include "video_decoder.h"

using namespace std;

struct JPEGChunk {
  int chunkSize;
  char *frameData;
  bool dirty;
};

class JPEGChunkWrapper : public Chunk {
  private:
    JPEGChunk *c;
  public:
    JPEGChunkWrapper(JPEGChunk *c): c(c) {};
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

class JPEGDecoder : public VideoDecoder {
  private:
    string filePath;
    FILE *f;
    // Lock around filehandler access
    mutex mtx; 

    JPEGChunk *frameChunks;

    int nextFrame = 1;
    int nextOffset = 0;
    char capacity = 100;
    int totalFrames;
    int fileSize;
    int width;
    int height;

  public:
    JPEGDecoder(string filePath): filePath(filePath) {
      // int fps = ffmpeg -i ../vid.mp4 2>&1 | sed -n "s/.*, \(.*\) fp.*/\1/p"
      // ffmpeg -r [fps] -i vid.mp4 -f image2 image-%3d.jpeg
      // ffmpeg -i vid.mp4 audio.mp3
      // ffmpeg -r [fps] -i image-%3d.jpeg -i audio.mp3 -codec copy output.mkv
    };
    virtual ~JPEGDecoder() {
      this->writeBack();
      free(this->frameChunks);
      fclose(this->f);
    };
    virtual void writeBack() {
      // Lock needed for the case in which flush is called twice in quick sucsession
      mtx.lock();
      printf("Writing back to disc...\n");
      int i = 0;
      while (i < this->totalFrames) {
        loadBar(i, this->totalFrames - 1, 50);
        i ++;
      }
      mtx.unlock();
    };
    virtual Chunk *getFrame(int frame) {
      return new JPEGChunkWrapper(&this->frameChunks[frame]); 
    };                                     
    virtual int getFileSize() {
      return this->fileSize; 
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
   virtual int frameSize() {
     return (int)floor(this->width * this->height * 3 * (this->capacity / 100.0));
   };
};
