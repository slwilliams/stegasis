#include <stdio.h>
#include <string.h>
extern "C" {
//  #include "libjpeg/jpeglib.h"
//  #include "libjpeg/transupp.h"  
}

#include "steganographic_algorithm.h"
#include "video/video_decoder.h"

class LDCTAlgorithm : public SteganographicAlgorithm {
  public:
    LDCTAlgorithm(VideoDecoder *dec) {
      this->dec = dec;
    };
    void getCoef(int frameByte, int *row, int *block, int *co) {
      *row = frameByte / (DCTSIZE2 * this->dec->frameWidth());
      *block = (frameByte - *row * this->dec->frameWidth() * DCTSIZE2) / DCTSIZE2;
      if (*block < 0) *block = 0;
      *co = frameByte % DCTSIZE2;
    };
    virtual void embed(Chunk *c, char *data, int dataBytes, int offset) {
      if (dataBytes == 0) return;
      offset /= 2;
      int frameByte = offset;
      int row, block, co;
      JBLOCKARRAY frame1;
      JBLOCKARRAY frame2;
      for (int i = 0; i < dataBytes; i ++) {
        for (int j = 7; j >= 0; j --) {
          this->getCoef(frameByte++, &row, &block, &co);
          frame1 = (JBLOCKARRAY)c->getFrameData(row, 1);
          if ((((1 << j) & data[i]) >> j) == 1) {
            frame1[0][block][co] |= 1;
          } else {
            frame1[0][block][co] &= ~1;
          }
          j --;
          frame2 = (JBLOCKARRAY)c->getFrameData(row, 2);
          if ((((1 << j) & data[i]) >> j) == 1) {
            frame2[0][block][co] |= 1;
          } else {
            frame2[0][block][co] &= ~1;
          }
        }
      }
    };
    virtual void extract(Chunk *c, char *output, int dataBytes, int offset) {
      if (dataBytes == 0) return;
      offset /= 2;
      int frameByte = offset;
      int row, block, co;
      JBLOCKARRAY frame1;
      JBLOCKARRAY frame2;
      for (int i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (int j = 7; j >= 0; j --) {
          this->getCoef(frameByte++, &row, &block, &co);
          frame1 = (JBLOCKARRAY)c->getFrameData(row, 1);
          output[i] |= ((frame1[0][block][co] & 1) << j);            
          j --;
          frame2 = (JBLOCKARRAY)c->getFrameData(row, 2);
          output[i] |= ((frame2[0][block][co] & 1) << j);            
        }
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'D', 'C', 'T'};
      memcpy(out, tmp, 4);
    };
};
