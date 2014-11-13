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


class JPEGDecoder : public VideoDecoder {
  private:
    string filePath;
    FILE *f;
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


      printf("Loading frames...\n");
      // Load each JPEG frame into ram:
      FILE *fp = NULL;
      int i = 0;
      this->frameChunks = (struct JPEGChunk *)malloc(sizeof(struct JPEGChunk) * this->totalFrames);
      for (i = 1; i <= this->totalFrames; i ++) {
        loadBar(i, this->totalFrames, 50);
        string fileName = "/tmp/output/image-" + std::to_string(i) + ".jpeg";
        fp = fopen(fileName.c_str(), "rb");

        struct jpeg_decompress_struct srcinfo;
        struct jpeg_compress_struct dstinfo;
        struct jpeg_error_mgr jsrcerr, jdsterr;

        static jpeg_transform_info transformoption; /* image transformation options */
        transformoption.transform = JXFORM_NONE;
        transformoption.trim = FALSE;
        transformoption.force_grayscale = FALSE;

        jvirt_barray_ptr *src_coef_arrays;
        jvirt_barray_ptr *dst_coef_arrays;

        /* Initialize the JPEG decompression object with default error handling. */
        //srcinfo.err = jpeg_std_error(&jsrcerr);
        this->frameChunks[i-1].srcinfo.err = jpeg_std_error(&jsrcerr);

        //jpeg_create_decompress(&srcinfo);
        jpeg_create_decompress(&this->frameChunks[i-1].srcinfo);
        /* Initialize the JPEG compression object with default error handling. */
        this->frameChunks[i-1].dstinfo.err = jpeg_std_error(&jdsterr);
        //jpeg_create_compress(&dstinfo);
        jpeg_create_compress(&this->frameChunks[i-1].dstinfo);

        /* Specify data source for decompression */
        //jpeg_stdio_src(&srcinfo, fp);
        jpeg_stdio_src(&this->frameChunks[i-1].srcinfo, fp);

        /* Enable saving of extra markers that we want to copy */
        //jcopy_markers_setup(&srcinfo, JCOPYOPT_ALL);
        jcopy_markers_setup(&this->frameChunks[i-1].srcinfo, JCOPYOPT_ALL);

        /* Read file header */
        //(void)jpeg_read_header(&srcinfo, TRUE);
        (void)jpeg_read_header(&this->frameChunks[i-1].srcinfo, TRUE);

        //jtransform_request_workspace(&srcinfo, &transformoption);
        jtransform_request_workspace(&this->frameChunks[i-1].srcinfo, &transformoption);
        //src_coef_arrays = jpeg_read_coefficients(&srcinfo);
        this->frameChunks[i-1].src_coef_arrays = jpeg_read_coefficients(&this->frameChunks[i-1].srcinfo);
        //jpeg_copy_critical_parameters(&srcinfo, &dstinfo);
        jpeg_copy_critical_parameters(&this->frameChunks[i-1].srcinfo, &this->frameChunks[i-1].dstinfo);

        //this->storeDCT(i-1, &srcinfo, &dstinfo, src_coef_arrays);
        this->storeDCT(i-1, &this->frameChunks[i-1].srcinfo, &this->frameChunks[i-1].dstinfo, this->frameChunks[i-1].src_coef_arrays);

        // ..when done with DCT, do this:
        //dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo, src_coef_arrays, &transformoption);
        this->frameChunks[i-1].dst_coef_arrays = jtransform_adjust_parameters(&this->frameChunks[i-1].srcinfo, &this->frameChunks[i-1].dstinfo, this->frameChunks[i-1].src_coef_arrays, &transformoption);
        fclose(fp);

        // And write everything back
        fp = fopen(fileName.c_str(), "wb");

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

        fclose(fp);
      }

      // ffmpeg -r [fps] -i /tmp/output/image-%3d.jpeg -i /tmp/output/audio.mp3 -codec copy output.mkv
      string muxCommand = "ffmpeg -r " + to_string(this->fps) + " -i /tmp/output/image-%d.jpeg -i /tmp/output/audio.wav -codec copy output.mkv";
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
    void storeDCT(int frame, j_decompress_ptr srcinfo, j_compress_ptr dstinfo, jvirt_barray_ptr *src_coef_arrays) {
      size_t block_row_size;
      JBLOCKARRAY *coef_buffers = (JBLOCKARRAY *)malloc(sizeof(JBLOCKARRAY) * srcinfo->num_components);
      JBLOCKARRAY *row_ptrs = (JBLOCKARRAY *)malloc(sizeof(JBLOCKARRAY) * srcinfo->num_components);

      //printf("MAX_COM: %d, nun_comp: %d\n", MAX_COMPONENTS, srcinfo->num_components);

      // Allocate DCT array buffers
      for (JDIMENSION compnum = 0; compnum < srcinfo->num_components; compnum ++) {
        coef_buffers[compnum] = (dstinfo->mem->alloc_barray)((j_common_ptr)dstinfo, JPOOL_IMAGE,
                                srcinfo->comp_info[compnum].width_in_blocks, srcinfo->comp_info[compnum].height_in_blocks);
      }

      // For each component,
      for (JDIMENSION compnum = 0; compnum < srcinfo->num_components; compnum ++) {
        block_row_size = (size_t)sizeof(JCOEF)*DCTSIZE2*srcinfo->comp_info[compnum].width_in_blocks;
        // ...iterate over rows,
        for (JDIMENSION rownum = 0; rownum < srcinfo->comp_info[compnum].height_in_blocks; rownum ++) {
            row_ptrs[compnum] = ((dstinfo)->mem->access_virt_barray)((j_common_ptr)&dstinfo, src_coef_arrays[compnum],
                                rownum, (JDIMENSION) 1, FALSE);
            // ...and for each block in a row,
            for (JDIMENSION blocknum = 0; blocknum < srcinfo->comp_info[compnum].width_in_blocks; blocknum ++) {
                // ...iterate over DCT coefficients
                for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
                        // Manipulate your DCT coefficients here. For instance, the code here inverts the image.
                        coef_buffers[compnum][rownum][blocknum][i] = -row_ptrs[compnum][0][blocknum][i];
                }
            }
        }
      }

      // Save the changes
      for (JDIMENSION compnum = 0; compnum < srcinfo->num_components; compnum ++) {
        block_row_size = (size_t)sizeof(JCOEF)*DCTSIZE2 * srcinfo->comp_info[compnum].width_in_blocks;
        // ...iterate over rows
        for (JDIMENSION rownum = 0; rownum < srcinfo->comp_info[compnum].height_in_blocks; rownum ++) {
            // Copy the whole rows
            row_ptrs[compnum] = (dstinfo->mem->access_virt_barray)((j_common_ptr)dstinfo, src_coef_arrays[compnum],
                                 rownum, (JDIMENSION) 1, TRUE);
            memcpy(row_ptrs[compnum][0][0], coef_buffers[compnum][rownum][0], block_row_size);
        }
      }   
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
