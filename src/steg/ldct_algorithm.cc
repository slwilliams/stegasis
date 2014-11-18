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
      offset *= 2;
      JQUANT_TBL *table = (JQUANT_TBL *)c->getChunkSize();
      int dataByte = 0;
      int j = 7;
      char mask = 1;
     /* for (JDIMENSION blocknum = 0; blocknum < 30; blocknum ++) {
        printf("embedd::=-------\n");
        for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
          printf("%d, ", frame[0][blocknum][i]);
        }
        printf("\n");
        break;
      }*/ 

      int row = offset / (64 * 30);
      int block = offset / 64;
      int co = offset % 64;
      // c[0] > c[1] -> 1
      for (int r = row; r < 23; r ++) {
        JBLOCKARRAY frame = (JBLOCKARRAY)c->getFrameData(r);
        for (JDIMENSION blocknum = block; blocknum < 30; blocknum ++) {
          for (JDIMENSION i = co; i < DCTSIZE2; i += 2) {
            int c1 = frame[0][blocknum][i];
            int c2 = frame[0][blocknum][i+1];
            if ((((mask << j) & data[dataByte]) >> j) == 1) {
              if (c1 <= c2) {
                if (c1 < c2) {
                  frame[0][blocknum][i] = c2;
                  frame[0][blocknum][i+1] = c1;
                } else {
                  // c1 == c2
                  // force c1 to be bigger than c2
                  frame[0][blocknum][i] += (int)table->quantval[i];
                }
              } 
            } else {
              if (c1 > c2) {
                frame[0][blocknum][i] = c2;
                frame[0][blocknum][i+1] = c1;
              }
            }
            j --;
            if (j == -1) {
              j = 7;
              dataByte ++;
              if (dataByte == dataBytes) {
            /*    for (JDIMENSION blocknum = 0; blocknum < 30; blocknum ++) {
                  printf("embeddAFTYER::=-------\n");
                  for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
                    printf("%d, ", frame[0][blocknum][i]);
                  }
                  printf("\n");
                  break;
                }*/ 
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
      offset *= 2;
      int dataByte = 0;
      int j = 7;
      int row = offset / (64 * 30);
      int block = offset / 64;
      int co = offset % 64;
      for (int r = row; r < 23; r ++) {
        JBLOCKARRAY frame = (JBLOCKARRAY)c->getFrameData(r);
        for (JDIMENSION blocknum = block; blocknum < 30; blocknum ++) {
          for (JDIMENSION i = co; i < DCTSIZE2; i += 2) {
            if (frame[0][blocknum][i] > frame[0][blocknum][i+1]) {
              // Data bit is 1
              output[dataByte] |= (1 << j);
            } else {
              // Data bit is 0
              output[dataByte] &= ~(1 << j);
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
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'D', 'C', 'T'};
      memcpy(out, tmp, 4);
    };
};
