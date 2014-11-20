#include <stdio.h>
#include <string.h>
extern "C" {
//  #include "libjpeg/jpeglib.h"
//  #include "libjpeg/transupp.h"  
}

#include "steganographic_algorithm.h"

class LDCTAlgorithm : public SteganographicAlgorithm {
  public:
    virtual void embed(Chunk *c, char *data, int dataBytes, int offset) {
      if (dataBytes == 0) return;
      offset /= 2;
      int dataByte = 0;
      int j = 7;
      int row = offset / (64 * 80);
      int block = (offset - row * 80 * 64) / 64;
      if (block < 0) block = 0;
      int co = offset % 64;
      for (int r = row; r < 45; r ++) {
        JBLOCKARRAY frame1 = (JBLOCKARRAY)c->getFrameData(r, 1);
        JBLOCKARRAY frame2 = (JBLOCKARRAY)c->getFrameData(r, 2);
        for (JDIMENSION blocknum = block; blocknum < 80; blocknum ++) {
          for (JDIMENSION i = co; i < DCTSIZE2; i += 1) {
            if ((((1 << j) & data[dataByte]) >> j) == 1) {
              frame1[0][blocknum][i] |= 1;
            } else {
              frame1[0][blocknum][i] &= ~1;
            }
            j --;
            if ((((1 << j) & data[dataByte]) >> j) == 1) {
              frame2[0][blocknum][i] |= 1;
            } else {
              frame2[0][blocknum][i] &= ~1;
            }
            j --;
            if (j == -1) {
              j = 7;
              dataByte ++;
              if (dataByte == dataBytes) {
                return;
              }
            }
          }
          co = 0;
        }
        block = 0;
      }
    };
    virtual void extract(Chunk *c, char *output, int dataBytes, int offset) {
      if (dataBytes == 0) return;
      offset /= 2;
      int dataByte = 0;
      int j = 7;
      int row = offset / (64 * 80);
      int block = (offset - row * 80 * 64) / 64;
      if (block < 0) block = 0;
      int co = offset % 64;
      output[dataByte] = 0;
      // TODO: not hard code these
      for (int r = row; r < 45; r ++) {
        JBLOCKARRAY frame1 = (JBLOCKARRAY)c->getFrameData(r, 1);
        JBLOCKARRAY frame2 = (JBLOCKARRAY)c->getFrameData(r, 2);
        for (JDIMENSION blocknum = block; blocknum < 80; blocknum ++) {
          for (JDIMENSION i = co; i < DCTSIZE2; i += 1) {
            output[dataByte] |= ((frame1[0][blocknum][i] & 1) << j);            
            j --;
            output[dataByte] |= ((frame2[0][blocknum][i] & 1) << j);            
            j --;
            if (j == -1) {
              j = 7;
              dataByte ++;
              output[dataByte] = 0;
              if (dataByte == dataBytes) {
                return;
              }
            }
          }
          co = 0;
        }
        block = 0;
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'D', 'C', 'T'};
      memcpy(out, tmp, 4);
    };
};
