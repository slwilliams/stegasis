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

string exec(char *cmd) {
    FILE *pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    string result = "";
    while (!feof(pipe)) {
      if (fgets(buffer, 128, pipe) != NULL)
        result += buffer;
    }
    pclose(pipe);
    return result;
}

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
    int fps;

  public:
    JPEGDecoder(string filePath): filePath(filePath) {
      // int fps = ffmpeg -i ../vid.mp4 2>&1 | sed -n "s/.*, \(.*\) fp.*/\1/p"
      string fpsCommand = "ffmpeg -i " + filePath + " 2>&1 | sed -n \"s/.*, \\(.*\\) fp.*/\\1/p\"";
      this->fps = (int)floor(atof(exec((char *)fpsCommand.c_str()).c_str()));
      printf("fps: %d\n", this->fps);

      // Make /tmp/output
      string mkdir = "mkdir /tmp/output";
      exec((char *)mkdir.c_str());
      // Remove any files
      string rm = "rm -rf /tmp/output/*";
      exec((char *)rm.c_str());

      // ffmpeg -r [fps] -i vid.mp4 -qscale:v 2 -f image2 /tmp/output/image-%3d.jpeg
      string extractCommand = "ffmpeg -r " + to_string(this->fps) + " -i " + filePath + " -qscale:v 2 -f image2 /tmp/output/image-%3d.jpeg";
      exec((char *)extractCommand.c_str());

      // Get total number of frames
      string totalFramesCommand = "ls /tmp/output | wc -l";
      this->totalFrames = atoi(exec((char *)totalFramesCommand.c_str()).c_str());
      printf("totalframes: %d\n", this->totalFrames);

      // ffmpeg -i vid.mp4 /tmp/output/audio.mp3
      // TODO: Change this to .mp3
      string getAudioCommand = "ffmpeg -i " + filePath + " /tmp/output/audio.wav";
      exec((char *)getAudioCommand.c_str());

      // ffmpeg -r [fps] -i /tmp/output/image-%3d.jpeg -i /tmp/output/audio.mp3 -codec copy output.mkv
      string muxCommand = "ffmpeg -r " + to_string(this->fps) + " -i /tmp/output/image-%3d.jpeg -i /tmp/output/audio.wav -codec copy output.mkv";
      exec((char *)muxCommand.c_str());
      exit(0);
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
