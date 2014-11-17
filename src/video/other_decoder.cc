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
    virtual long getChunkSize() {
      jpeg_component_info *ci_ptr = &this->c->srcinfo.comp_info[1];
      JQUANT_TBL *tbl = ci_ptr->quant_table;
      return (long)tbl;
    };
    virtual char *getFrameData() {
      char *out = (char *)this->c->dstinfo.mem->access_virt_barray((j_common_ptr)&this->c->dstinfo, this->c->src_coef_arrays[1], 
          0, (JDIMENSION)1, FALSE);
      return out;
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

    list<struct JPEGChunk> frameChunks;
    int *jpegSizes;
    unsigned char **jpegs;

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
      printf("size:::: %lu\n", sizeof(JQUANT_TBL *));
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
      //string extractCommand = "ffmpeg -r " + to_string(this->fps) + " -i " + filePath + " -qscale:v 2 -f image2 /tmp/output/image-%d.jpeg";
      string extractCommand = "ffmpeg -r " + to_string(this->fps) + " -i " + filePath + " -vcodec copy /tmp/output/image-%d.jpeg";
      exec((char *)extractCommand.c_str());

      // Get total number of frames
      string totalFramesCommand = "ls /tmp/output | wc -l";
      this->totalFrames = atoi(exec((char *)totalFramesCommand.c_str()).c_str());
      printf("totalframes: %d\n", this->totalFrames);

      // ffmpeg -i vid.mp4 /tmp/output/audio.mp3
      // TODO: Change this to .mp3
      string getAudioCommand = "ffmpeg -i " + filePath + " /tmp/output/audio.wav";
      exec((char *)getAudioCommand.c_str());

      // Set the global transform options
      transformoption.transform = JXFORM_NONE;
      transformoption.trim = FALSE;
      transformoption.force_grayscale = FALSE;

      this->jpegSizes = (int *)malloc(sizeof(int) * this->totalFrames);
      this->jpegs = (unsigned char **)malloc(sizeof(unsigned char *) * this->totalFrames);

      printf("Reading JPEGs into ram...\n");
      int i = 0;
      int read = 0;
      for (i = 0; i < this->totalFrames; i ++) {
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

      printf("modifiying frames...\n");
      for (i = 0; i < 1; i ++) {
        this->getFrame(i);
        //this->storeDCT(&this->frameChunks.back().srcinfo, &this->frameChunks.back().dstinfo, this->frameChunks.back().src_coef_arrays);
      }
    };
    virtual ~JPEGDecoder() {
      this->writeBack();
      this->mux();
    };
    void mux() {
      printf("muxing\n");
      int i = 0;
      int read = 0;
      FILE *fp = NULL;
      printf("Writing back to disk...\n");
      for (i = 0; i < this->totalFrames; i ++) {
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
      if (this->frameChunks.size() == 0) return;
      mtx.lock();
      printf("wirteh=back\n");
      for (struct JPEGChunk c : this->frameChunks) {
        jtransform_request_workspace(&c.srcinfo, &transformoption);
        jpeg_component_info *ci_ptr = &c.srcinfo.comp_info[1];
        c.dst_coef_arrays = jtransform_adjust_parameters(&c.srcinfo, &c.dstinfo, c.src_coef_arrays, &transformoption);

        // Specify data destination for compression
        unsigned long size = (unsigned long)this->jpegSizes[c.frame];
        jpeg_mem_dest(&c.dstinfo, &this->jpegs[c.frame], &size);

        // Start compressor (note no image data is actually written here)
        jpeg_write_coefficients(&c.dstinfo, c.dst_coef_arrays);

        // Copy to the output file any extra markers that we want to preserve 
        jcopy_markers_execute(&c.srcinfo, &c.dstinfo, JCOPYOPT_ALL);
        
        // THISOPNE----
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
      jpeg_component_info *ci_ptr = &srcinfo->comp_info[2];
      JQUANT_TBL *tbl = ci_ptr->quant_table;
      /*printf("quantization table: %d\n", 2);
      for (int i = 0; i < DCTSIZE2; i ++) {
          printf("% 4d ", (int)(tbl->quantval[i]));
          if ((i + 1) % 8 == 0)
            printf("\n");
      }*/

      JBLOCKARRAY *row_ptrs = (JBLOCKARRAY *)malloc(sizeof(JBLOCKARRAY) * srcinfo->num_components);
      printf("height: %d, width: %d\n", srcinfo->comp_info[2].height_in_blocks, srcinfo->comp_info[2].width_in_blocks);

      int compnum = 1;
      // Only use component 2
      // Use 23 instead of 1 to get all blocks
      // row_ptrs is then row_ptr[rownum][blocknum][i] don't even need compnum
      
      // For each component,
     // for (JDIMENSION compnum = 0; compnum < srcinfo->num_components; compnum ++) {
        // ...iterate over rows,
        for (JDIMENSION rownum = 0; rownum < srcinfo->comp_info[compnum].height_in_blocks; rownum ++) {
          row_ptrs[compnum] = dstinfo->mem->access_virt_barray((j_common_ptr)&dstinfo, src_coef_arrays[compnum], 
              rownum, (JDIMENSION)1, FALSE);
          // ...and for each block in a row,
          for (JDIMENSION blocknum = 0; blocknum < srcinfo->comp_info[compnum].width_in_blocks; blocknum ++) {
            // ...iterate over DCT coefficients
            // Don't modify the DC term
            for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
              // Manipulate your DCT coefficients here. For instance, the code here inverts the image.
              //coef_buffers[compnum][rownum][blocknum][i] = -row_ptrs[compnum][0][blocknum][i];
              printf("%d, ", row_ptrs[compnum][0][blocknum][i]);
              if (row_ptrs[compnum][0][blocknum][i] > 0) { 
              //  row_ptrs[compnum][0][blocknum][i] += 1;
              }
            }
            printf("\n");
            return;
          }
        }
      //}
    };
    // TODO: possbile concurrent isses with .back()...
    virtual Chunk *getFrame(int frame) {
     // if (this->frameChunks.size() > 100) {
        this->writeBack();
    //  }
      //string fileName = "/tmp/output/image-" + std::to_string(frame+1) + ".jpeg";
      //FILE *fp = fopen(fileName.c_str(), "rb");
      mtx.lock();
      struct JPEGChunk c;
      c.frame = frame;
      this->frameChunks.push_back(c);

      this->frameChunks.back().srcinfo.err = jpeg_std_error(&this->frameChunks.back().jsrcerr);
      jpeg_create_decompress(&this->frameChunks.back().srcinfo);

      this->frameChunks.back().dstinfo.err = jpeg_std_error(&this->frameChunks.back().jdsterr);
      jpeg_create_compress(&this->frameChunks.back().dstinfo);

      // Specify data source for decompression
      jpeg_mem_src(&this->frameChunks.back().srcinfo, this->jpegs[frame], this->jpegSizes[frame]);
      jpeg_set_quality(&this->frameChunks.back().dstinfo, 100, FALSE);

      jcopy_markers_setup(&this->frameChunks.back().srcinfo, JCOPYOPT_ALL);

      (void)jpeg_read_header(&this->frameChunks.back().srcinfo, TRUE);

      jtransform_request_workspace(&this->frameChunks.back().srcinfo, &transformoption);
      this->frameChunks.back().src_coef_arrays = jpeg_read_coefficients(&this->frameChunks.back().srcinfo);
      jpeg_copy_critical_parameters(&this->frameChunks.back().srcinfo, &this->frameChunks.back().dstinfo);

      mtx.unlock();
      return new JPEGChunkWrapper(&this->frameChunks.back()); 
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
      struct JPEGChunk c;
      if (this->frameChunks.size() > 0) {
        c = this->frameChunks.front();
      } else {
        // This will cause frameChunks to get an element
        this->getFrame(0);
        c = this->frameChunks.front();
      }
      return c.srcinfo.comp_info[1].height_in_blocks * c.srcinfo.comp_info[1].width_in_blocks * 64 * 4;
    };
};
