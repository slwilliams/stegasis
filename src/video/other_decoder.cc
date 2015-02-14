#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>
#include <mutex> 
#include <math.h>
#include <sys/stat.h>

extern "C" {
  #include <libjpeg/jpeglib.h>
  #include <libjpeg/transupp.h>  
}

#include "common/progress_bar.h"
#include "video_decoder.h"

using namespace std;

struct JPEGFrame {
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  jvirt_barray_ptr *src_coef_arrays;
  jvirt_barray_ptr *dst_coef_arrays;
  struct jpeg_error_mgr jsrcerr, jdsterr;
  bool dirty;
  int frame;
};

class JPEGFrameWrapper : public Frame {
  private:
    JPEGFrame *c;
  public:
    JPEGFrameWrapper(JPEGFrame *c): c(c) {};
    virtual long getFrameSize() {
      return this->frameSize;
    };
    virtual char *getFrameData(int n, int c) {
      return (char *)this->c->dstinfo.mem->access_virt_barray((j_common_ptr)&this->c->dstinfo,
         this->c->dst_coef_arrays[c], n, (JDIMENSION)1, FALSE);
    };
    virtual bool isDirty() {
      return this->c->dirty;
    };
    virtual void setDirty() {
      this->c->dirty = 1;
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
    mutex mtx; 

    jpeg_transform_info transformoption;
    list<struct JPEGFrame> frames;
    int *jpegSizes;
    unsigned char **jpegs;
    bool format;
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
    JPEGDecoder(string filePath, bool format): filePath(filePath), format(format) {
      string fpsCommand = "ffmpeg -i " + filePath + " 2>&1 | sed -n \"s/.*, \\(.*\\) fp.*/\\1/p\"";
      this->fps = (int)floor(atof(exec((char *)fpsCommand.c_str()).c_str()));

      string mkdir = "mkdir /tmp/output 2>&1";
      exec((char *)mkdir.c_str());
      string rm = "rm -rf /tmp/output/*";
      exec((char *)rm.c_str());

      string extractCommand;
      if (this->format) {
        extractCommand = "ffmpeg -v quiet -stats -r " + to_string(this->fps) + " -i " + filePath + " -qscale:v 2 -f image2 /tmp/output/image-%d.jpeg";
      } else {
        extractCommand = "ffmpeg -v quiet -stats -r " + to_string(this->fps) + " -i " + filePath + " -vcodec copy /tmp/output/image-%d.jpeg";
      }
      printf("Extracting JPEG frames...\n");
      exec((char *)extractCommand.c_str());
      printf("\n");

      string totalFramesCommand = "ls /tmp/output | wc -l";
      this->totalFrames = atoi(exec((char *)totalFramesCommand.c_str()).c_str());

      printf("Extracting audio...\n");
      string getAudioCommand;
      if (this->format) {
        getAudioCommand = "ffmpeg -v quiet -stats -i " + filePath + " /tmp/output/audio.mp3";
      } else {
        getAudioCommand = "ffmpeg -v quiet -stats -i " + filePath + " -vn -acodec copy /tmp/output/audio.mp3";
      }
      exec((char *)getAudioCommand.c_str());
      printf("\n");

      transformoption.transform = JXFORM_NONE;
      transformoption.trim = FALSE;
      transformoption.force_grayscale = FALSE;

      this->jpegSizes = (int *)malloc(sizeof(int) * this->totalFrames);
      this->jpegs = (unsigned char **)malloc(sizeof(unsigned char *) * this->totalFrames);

      printf("Reading JPEGs into ram...\n");
      int read = 0;
      for (int i = 0; i < this->totalFrames; i ++) {
        loadBar(i, this->totalFrames - 1, 50);
        string fileName = "/tmp/output/image-" + to_string(i+1) + ".jpeg";
        FILE *fp = fopen(fileName.c_str(), "rb");
        struct stat file_info;
        stat(fileName.c_str(), &file_info);
        this->jpegSizes[i] = file_info.st_size;
        this->jpegs[i] = (unsigned char *)malloc(sizeof(unsigned char) * file_info.st_size);   
        read = fread(this->jpegs[i], file_info.st_size, 1, fp);
        fclose(fp);
      }
      printf("\n");

      // Cause frames to get an element
      this->getFrame(0);
      struct JPEGFrame f = this->frames.front();
      this->width = f.srcinfo.comp_info[1].width_in_blocks;
      this->height = f.srcinfo.comp_info[1].height_in_blocks;
    };
    virtual ~JPEGDecoder() {
      this->writeBack();
      this->mux();
    };
    void mux() {
      int read = 0;
      FILE *fp = NULL;
      printf("Writing back to disk...\n");
      for (int i = 0; i < this->totalFrames; i ++) {
        loadBar(i, this->totalFrames - 1, 50);
        string fileName = "/tmp/output/image-" + to_string(i+1) + ".jpeg";
        fp = fopen(fileName.c_str(), "wb");
        read = fwrite(this->jpegs[i], 1, this->jpegSizes[i], fp);
        fclose(fp);
      }
      printf("\n");
      string newFilePath = this->filePath.substr(0, this->filePath.find_last_of(".") + 1) + "mkv";
      string muxCommand = "ffmpeg -v quiet -stats -y -framerate " + to_string(this->fps) + " -i /tmp/output/image-%d.jpeg -i /tmp/output/audio.mp3 -codec copy -shortest " + newFilePath;
      printf("Muxing video to %s...\n", newFilePath.c_str());
      exec((char *)muxCommand.c_str());
      printf("\n");
    };
    virtual void writeBack() {
      // Lock needed for the case in which flush is called twice in quick sucsession
      mtx.lock();
      for (struct JPEGFrame f : this->frames) {
        printf("writing back frame: %d\n", f.frame);
        unsigned long size = (unsigned long)this->jpegSizes[f.frame];
        jpeg_mem_dest(&f.dstinfo, &this->jpegs[f.frame], &size);

        jpeg_write_coefficients(&f.dstinfo, f.dst_coef_arrays);
        jcopy_markers_execute(&f.srcinfo, &f.dstinfo, JCOPYOPT_ALL);
        
        jpeg_finish_compress(&f.dstinfo);
        jpeg_destroy_compress(&f.dstinfo);
        jpeg_finish_decompress(&f.srcinfo);
        jpeg_destroy_decompress(&f.srcinfo);

        this->jpegSizes[f.frame] = (int)size;
      }
      this->frames.clear();
      mtx.unlock();
    };
    virtual Frame *getFrame(int frame) {
      this->writeBack();

      mtx.lock();
      struct JPEGFrame f;
      f.frame = frame;
      this->frames.push_back(f);

      this->frames.back().srcinfo.err = jpeg_std_error(&this->frames.back().jsrcerr);
      jpeg_create_decompress(&this->frames.back().srcinfo);

      this->frames.back().dstinfo.err = jpeg_std_error(&this->frames.back().jdsterr);
      jpeg_create_compress(&this->frames.back().dstinfo);

      jpeg_mem_src(&this->frames.back().srcinfo, this->jpegs[frame], this->jpegSizes[frame]);
      jpeg_set_quality(&this->frames.back().dstinfo, 100, FALSE);

      jcopy_markers_setup(&this->frames.back().srcinfo, JCOPYOPT_ALL);
      jpeg_read_header(&this->frames.back().srcinfo, TRUE);

      jtransform_request_workspace(&this->frames.back().srcinfo, &transformoption);
      this->frames.back().src_coef_arrays = jpeg_read_coefficients(&this->frames.back().srcinfo);
      jpeg_copy_critical_parameters(&this->frames.back().srcinfo, &this->frames.back().dstinfo);

      jtransform_request_workspace(&this->frames.back().srcinfo, &transformoption);
      this->frames.back().dst_coef_arrays = jtransform_adjust_parameters(&this->frames.back().srcinfo, &this->frames.back().dstinfo, this->frames.back().src_coef_arrays, &transformoption);

      mtx.unlock();
      return new JPEGFrameWrapper(&this->frames.back()); 
    };                                     
    virtual Frame *getHeaderFrame() {
      if (this->hidden) {
        return this->getFrame(this->getNumberOfFrames() / 2);
      } else {
        return this->getFrame(0);
      }
    };
    virtual int getFileSize() {
      return this->jpegSizes[0]; 
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
      return (int)floor(this->width * this->height * 63 * (capacity / 100.0));
    };
};
