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
      //printf("embedd: bytes: %d, offset: %d\n", dataBytes, offset);
      if (dataBytes == 0) return;
      int dataByte = 0;
      int j = 7;
      int row = offset / (64 * 30);
      int block = (offset - row * 30 * 64) / 64;
      if (block < 0) block = 0;
      int co = offset % 64;
      //printf("row: %d, block: %d, co: %d\n", row, block, co);
      for (int r = row; r < 23; r ++) {
        JBLOCKARRAY frame1 = (JBLOCKARRAY)c->getFrameData(r, 1);
        for (JDIMENSION blocknum = block; blocknum < 30; blocknum ++) {
          for (JDIMENSION i = co; i < DCTSIZE2; i += 1) {
            if ((((1 << j) & data[dataByte]) >> j) == 1) {
              frame1[0][blocknum][i] |= 1;
            } else {
              frame1[0][blocknum][i] &= ~1;
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
      //printf("extract: bytes: %d, offset: %d\n", dataBytes, offset);
      if (dataBytes == 0) return;
      int dataByte = 0;
      int j = 7;
      int row = offset / (64 * 30);
      int block = (offset - row * 30 * 64) / 64;
      if (block < 0) block = 0;
      int co = offset % 64;
      //printf("row: %d, block: %d, co: %d\n", row, block, co);
      output[dataByte] = 0;
      for (int r = row; r < 23; r ++) {
        JBLOCKARRAY frame1 = (JBLOCKARRAY)c->getFrameData(r, 1);
        for (JDIMENSION blocknum = block; blocknum < 30; blocknum ++) {
          for (JDIMENSION i = co; i < DCTSIZE2; i += 1) {
            output[dataByte] |= ((frame1[0][blocknum][i] & (1)) << j);            
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
      /*for (JDIMENSION blocknum = 0; blocknum < 30; blocknum ++) {
        printf("extarc::=-------\n");
        for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
          printf("%d, ", frame1[0][blocknum][i]);
        }
        printf("\n");
        break;
      } */
