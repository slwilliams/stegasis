#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <list>
#include <mutex> 
#include <math.h>
#include <sys/stat.h>

extern "C" {
  #include "libjpeg/jpeglib.h"
  #include "libjpeg/transupp.h"  
}
#include "common/progress_bar.h"
#include "video_decoder.h"

using namespace std;

struct JPEGChunk {
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  jvirt_barray_ptr *src_coef_arrays;
  jvirt_barray_ptr *dst_coef_arrays;
  struct jpeg_error_mgr jsrcerr, jdsterr;
  bool dirty;
  int frame;
};

class JPEGChunkWrapper : public Chunk {
  private:
    JPEGChunk *c;
  public:
    JPEGChunkWrapper(JPEGChunk *c): c(c) {};
    virtual void *getAdditionalData() {
      jpeg_component_info *ci_ptr = &this->c->srcinfo.comp_info[1];
      return (void *)ci_ptr->quant_table;
    };
    virtual long getChunkSize() {
      return this->chunkSize;
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

static jpeg_transform_info transformoption;

class JPEGDecoder : public VideoDecoder {
  private:
    string filePath;
    mutex mtx; 

    list<struct JPEGChunk> frameChunks;
    int *jpegSizes;
    unsigned char **jpegs;
    bool format;

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
      printf("fps: %d\n", this->fps);

      string mkdir = "mkdir /tmp/output";
      exec((char *)mkdir.c_str());
      string rm = "rm -rf /tmp/output/*";
      exec((char *)rm.c_str());

      string extractCommand;
      if (this->format) {
        extractCommand = "ffmpeg -r " + to_string(this->fps) + " -i " + filePath + " -qscale:v 1 -f image2 /tmp/output/image-%d.jpeg";
      } else {
        extractCommand = "ffmpeg -r " + to_string(this->fps) + " -i " + filePath + " -vcodec copy /tmp/output/image-%d.jpeg";
      }
      exec((char *)extractCommand.c_str());

      string totalFramesCommand = "ls /tmp/output | wc -l";
      this->totalFrames = atoi(exec((char *)totalFramesCommand.c_str()).c_str());
      printf("totalframes: %d\n", this->totalFrames);

      // TODO: Change this to .mp3
      string getAudioCommand = "ffmpeg -i " + filePath + " /tmp/output/audio.wav";
      exec((char *)getAudioCommand.c_str());

      transformoption.transform = JXFORM_NONE;
      transformoption.trim = FALSE;
      transformoption.force_grayscale = FALSE;

      this->jpegSizes = (int *)malloc(sizeof(int) * this->totalFrames);
      this->jpegs = (unsigned char **)malloc(sizeof(unsigned char *) * this->totalFrames);

      printf("Reading JPEGs into ram...\n");
      int read = 0;
      for (int i = 0; i < this->totalFrames; i ++) {
        loadBar(i, this->totalFrames - 1, 50);
        string fileName = "/tmp/output/image-" + std::to_string(i+1) + ".jpeg";
        FILE *fp = fopen(fileName.c_str(), "rb");
        struct stat file_info;
        stat(fileName.c_str(), &file_info);
        this->jpegSizes[i] = file_info.st_size;
        this->jpegs[i] = (unsigned char *)malloc(sizeof(unsigned char) * file_info.st_size);   
        read = fread(this->jpegs[i], file_info.st_size, 1, fp);
        fclose(fp);
      }

      struct JPEGChunk c;
      if (this->frameChunks.size() > 0) {
        c = this->frameChunks.front();
      } else {
        // This will cause frameChunks to get an element
        this->getFrame(0);
        c = this->frameChunks.front();
      }
      this->width = c.srcinfo.comp_info[1].width_in_blocks;
      this->height = c.srcinfo.comp_info[1].height_in_blocks;

     /* printf("modifiying frames...\n");
      for (i = 0; i < 1; i ++) {
        this->getFrame(i);
        this->storeDCT(&this->frameChunks.back().srcinfo, &this->frameChunks.back().dstinfo, this->frameChunks.back().src_coef_arrays);
      }*/
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
        string fileName = "/tmp/output/image-" + std::to_string(i+1) + ".jpeg";
        fp = fopen(fileName.c_str(), "wb");
        read = fwrite(this->jpegs[i], 1, this->jpegSizes[i], fp);
        fclose(fp);
      }
      string muxCommand = "ffmpeg -r " + to_string(this->fps) + " -i /tmp/output/image-%d.jpeg -i /tmp/output/audio.wav -codec copy output.mkv";
      exec((char *)muxCommand.c_str());
    };
    virtual void writeBack() {
      // Lock needed for the case in which flush is called twice in quick sucsession
      mtx.lock();
      for (struct JPEGChunk c : this->frameChunks) {
        unsigned long size = (unsigned long)this->jpegSizes[c.frame];
        jpeg_mem_dest(&c.dstinfo, &this->jpegs[c.frame], &size);

        jpeg_write_coefficients(&c.dstinfo, c.dst_coef_arrays);

        jcopy_markers_execute(&c.srcinfo, &c.dstinfo, JCOPYOPT_ALL);
        
        jpeg_finish_compress(&c.dstinfo);
        jpeg_destroy_compress(&c.dstinfo);
        (void)jpeg_finish_decompress(&c.srcinfo);
        jpeg_destroy_decompress(&c.srcinfo);

        this->jpegSizes[c.frame] = (int)size;
      }
      this->frameChunks.clear();
      mtx.unlock();
    };
    void storeDCT(j_decompress_ptr srcinfo, j_compress_ptr dstinfo, jvirt_barray_ptr *src_coef_arrays) {
      JBLOCKARRAY *row_ptrs = (JBLOCKARRAY *)malloc(sizeof(JBLOCKARRAY) * srcinfo->num_components);
      int compnum = 1;
     // for (JDIMENSION compnum = 0; compnum < srcinfo->num_components; compnum ++) {
        for (JDIMENSION rownum = 0; rownum < srcinfo->comp_info[compnum].height_in_blocks; rownum ++) {
          row_ptrs[compnum] = dstinfo->mem->access_virt_barray((j_common_ptr)&dstinfo, src_coef_arrays[compnum], 
              rownum, (JDIMENSION)1, FALSE);
          for (JDIMENSION blocknum = 0; blocknum < srcinfo->comp_info[compnum].width_in_blocks; blocknum ++) {
            for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
              printf("%d, ", row_ptrs[compnum][0][blocknum][i]);
            }
            printf("\n");
            return;
          }
        }
      //}
    };
    virtual Chunk *getFrame(int frame) {
      this->writeBack();

      mtx.lock();
      struct JPEGChunk c;
      c.frame = frame;
      this->frameChunks.push_back(c);

      this->frameChunks.back().srcinfo.err = jpeg_std_error(&this->frameChunks.back().jsrcerr);
      jpeg_create_decompress(&this->frameChunks.back().srcinfo);

      this->frameChunks.back().dstinfo.err = jpeg_std_error(&this->frameChunks.back().jdsterr);
      jpeg_create_compress(&this->frameChunks.back().dstinfo);

      jpeg_mem_src(&this->frameChunks.back().srcinfo, this->jpegs[frame], this->jpegSizes[frame]);
      jpeg_set_quality(&this->frameChunks.back().dstinfo, 100, FALSE);

      jcopy_markers_setup(&this->frameChunks.back().srcinfo, JCOPYOPT_ALL);
      (void)jpeg_read_header(&this->frameChunks.back().srcinfo, TRUE);

      jtransform_request_workspace(&this->frameChunks.back().srcinfo, &transformoption);
      this->frameChunks.back().src_coef_arrays = jpeg_read_coefficients(&this->frameChunks.back().srcinfo);
      jpeg_copy_critical_parameters(&this->frameChunks.back().srcinfo, &this->frameChunks.back().dstinfo);

      jtransform_request_workspace(&this->frameChunks.back().srcinfo, &transformoption);
      this->frameChunks.back().dst_coef_arrays = jtransform_adjust_parameters(&this->frameChunks.back().srcinfo, &this->frameChunks.back().dstinfo, this->frameChunks.back().src_coef_arrays, &transformoption);

      mtx.unlock();
      return new JPEGChunkWrapper(&this->frameChunks.back()); 
    };                                     
    virtual int getFileSize() {
      return 100;
      return this->jpegSizes[0]; 
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
      return (int)floor(this->width * this->height * 63 * (capacity / 100.0));
    };
};
