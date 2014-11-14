#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <mutex> 
#include <math.h>
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
};

class JPEGChunkWrapper : public Chunk {
  private:
    JPEGChunk *c;
  public:
    JPEGChunkWrapper(JPEGChunk *c): c(c) {};
    virtual int getChunkSize() {
      //return c->chunkSize;
    };
    virtual char *getFrameData() {
      //return c->frameData;
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

static jpeg_transform_info transformoption;

class JPEGDecoder : public VideoDecoder {
  private:
    string filePath;
    // Lock around filehandler access
    mutex mtx; 

    struct JPEGChunk *frameChunks;

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
      string extractCommand = "ffmpeg -r " + to_string(this->fps) + " -i " + filePath + " -qscale:v 2 -f image2 /tmp/output/image-%d.jpeg";
      exec((char *)extractCommand.c_str());

      // Get total number of frames
      string totalFramesCommand = "ls /tmp/output | wc -l";
      this->totalFrames = atoi(exec((char *)totalFramesCommand.c_str()).c_str());
      printf("totalframes: %d\n", this->totalFrames);

      // ffmpeg -i vid.mp4 /tmp/output/audio.mp3
      // TODO: Change this to .mp3
      string getAudioCommand = "ffmpeg -i " + filePath + " /tmp/output/audio.wav";
      exec((char *)getAudioCommand.c_str());

      // Load each JPEG frame into ram
      printf("Loading frames...\n");
      
      // Set the global transform options
      transformoption.transform = JXFORM_NONE;
      transformoption.trim = FALSE;
      transformoption.force_grayscale = FALSE;

      FILE *fp = NULL;
      int i = 0;
      this->frameChunks = (struct JPEGChunk *)malloc(sizeof(struct JPEGChunk) * this->totalFrames);
      for (i = 1; i <= this->totalFrames; i ++) {
        loadBar(i, this->totalFrames, 50);
        string fileName = "/tmp/output/image-" + std::to_string(i) + ".jpeg";
        fp = fopen(fileName.c_str(), "rb");

        this->frameChunks[i-1].srcinfo.err = jpeg_std_error(&this->frameChunks[i-1].jsrcerr);
        jpeg_create_decompress(&this->frameChunks[i-1].srcinfo);

        this->frameChunks[i-1].dstinfo.err = jpeg_std_error(&this->frameChunks[i-1].jdsterr);
        jpeg_create_compress(&this->frameChunks[i-1].dstinfo);

        /* Specify data source for decompression */
        jpeg_stdio_src(&this->frameChunks[i-1].srcinfo, fp);

        jcopy_markers_setup(&this->frameChunks[i-1].srcinfo, JCOPYOPT_ALL);

        (void)jpeg_read_header(&this->frameChunks[i-1].srcinfo, TRUE);

        jtransform_request_workspace(&this->frameChunks[i-1].srcinfo, &transformoption);
        this->frameChunks[i-1].src_coef_arrays = jpeg_read_coefficients(&this->frameChunks[i-1].srcinfo);
        jpeg_copy_critical_parameters(&this->frameChunks[i-1].srcinfo, &this->frameChunks[i-1].dstinfo);

        this->frameChunks[i-1].dirty = true;
        fclose(fp);
      }

      for (i = 100; i < 200; i ++) {
        this->storeDCT(i-1, &this->frameChunks[i-1].srcinfo, &this->frameChunks[i-1].dstinfo, this->frameChunks[i-1].src_coef_arrays);
      }
    };
    virtual ~JPEGDecoder() {
      this->writeBack();
      this->mux();
      free(this->frameChunks);
    };
    void mux() {
      string muxCommand = "ffmpeg -r " + to_string(this->fps) + " -i /tmp/output/image-%d.jpeg -i /tmp/output/audio.wav -codec copy output.mkv";
      exec((char *)muxCommand.c_str());
    };
    virtual void writeBack() {
      // Lock needed for the case in which flush is called twice in quick sucsession
      mtx.lock();
      printf("Writing back to disc...\n");
      FILE *fp = NULL;
      int i = 1;
      for (i = 1; i <= this->totalFrames; i ++) {
        if (this->frameChunks[i-1].dirty) {
          string fileName = "/tmp/output/image-" + std::to_string(i) + ".jpeg";
          loadBar(i, this->totalFrames, 50);

          fp = fopen(fileName.c_str(), "wb");
          jtransform_request_workspace(&this->frameChunks[i-1].srcinfo, &transformoption);
          this->frameChunks[i-1].dst_coef_arrays = jtransform_adjust_parameters(&this->frameChunks[i-1].srcinfo,
              &this->frameChunks[i-1].dstinfo, this->frameChunks[i-1].src_coef_arrays, &transformoption);

          // Specify data destination for compression
          jpeg_stdio_dest(&this->frameChunks[i-1].dstinfo, fp);

          // Start compressor (note no image data is actually written here)
          jpeg_write_coefficients(&this->frameChunks[i-1].dstinfo, this->frameChunks[i-1].dst_coef_arrays);

          // Copy to the output file any extra markers that we want to preserve 
          jcopy_markers_execute(&this->frameChunks[i-1].srcinfo, &this->frameChunks[i-1].dstinfo, JCOPYOPT_ALL);

          jpeg_finish_compress(&this->frameChunks[i-1].dstinfo);
          jpeg_destroy_compress(&this->frameChunks[i-1].dstinfo);
          (void)jpeg_finish_decompress(&this->frameChunks[i-1].srcinfo);
          jpeg_destroy_decompress(&this->frameChunks[i-1].srcinfo);

          this->frameChunks[i-1].dirty = false;
          fclose(fp);
        }
      }
      mtx.unlock();
    };
    void storeDCT(int frame, j_decompress_ptr srcinfo, j_compress_ptr dstinfo, jvirt_barray_ptr *src_coef_arrays) {
      size_t block_row_size;
      JBLOCKARRAY *row_ptrs = (JBLOCKARRAY *)malloc(sizeof(JBLOCKARRAY) * srcinfo->num_components);

      // For each component,
      for (JDIMENSION compnum = 0; compnum < srcinfo->num_components; compnum ++) {
        block_row_size = (size_t)sizeof(JCOEF)*DCTSIZE2*srcinfo->comp_info[compnum].width_in_blocks;
        // ...iterate over rows,
        for (JDIMENSION rownum = 0; rownum < srcinfo->comp_info[compnum].height_in_blocks; rownum ++) {
          row_ptrs[compnum] = dstinfo->mem->access_virt_barray((j_common_ptr)&dstinfo, src_coef_arrays[compnum], 
              rownum, (JDIMENSION)1, FALSE);
          // ...and for each block in a row,
          for (JDIMENSION blocknum = 0; blocknum < srcinfo->comp_info[compnum].width_in_blocks; blocknum ++) {
            // ...iterate over DCT coefficients
            for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
              // Manipulate your DCT coefficients here. For instance, the code here inverts the image.
              //coef_buffers[compnum][rownum][blocknum][i] = -row_ptrs[compnum][0][blocknum][i];
              row_ptrs[compnum][0][blocknum][i] *= -1;
            }
          }
        }
      }
    };
    virtual Chunk *getFrame(int frame) {
      int i = frame;
      jtransform_request_workspace(&this->frameChunks[i-1].srcinfo, &transformoption);
      this->frameChunks[i-1].src_coef_arrays = jpeg_read_coefficients(&this->frameChunks[i-1].srcinfo);
      jpeg_copy_critical_parameters(&this->frameChunks[i-1].srcinfo, &this->frameChunks[i-1].dstinfo);

      //this->storeDCT(i-1, &this->frameChunks[i-1].srcinfo, &this->frameChunks[i-1].dstinfo, this->frameChunks[i-1].src_coef_arrays);

      return new JPEGChunkWrapper(&this->frameChunks[i-1]); 
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
