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
      JBLOCKARRAY frame = (JBLOCKARRAY)c->getFrameData();
      int dataByte = 0;
      int j = 7;
      int frameByte = offset;
      char mask = 1;
      for (JDIMENSION blocknum = 0; blocknum < 30; blocknum ++) {
        printf("embedd::=-------\n");
        for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
          printf("%d, ", frame[0][blocknum][i]);
        }
        printf("\n");
        break;

      } 
      for (JDIMENSION blocknum = 0; blocknum < 30; blocknum ++) {
        for (JDIMENSION i = 0; i < DCTSIZE2; i ++) {
          if ((((mask << j) & data[dataByte]) >> j) == 1) {
            printf("before: %d\n", frame[0][blocknum][i]);
            frame[0][blocknum][i] |= 1 << 2;
            printf("after: %d\n", frame[0][blocknum][i]);
          } else {
            printf("before: %d\n", frame[0][blocknum][i]);
            frame[0][blocknum][i] &= ~(1 << 2);
            printf("after: %d\n", frame[0][blocknum][i]);
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
      }
    /*  int i = 0;
      int j = 0;
      int frameByte = offset;
      char mask = 1;
      for (i = 0; i < dataBytes; i ++) {
        for (j = 7; j >= 0; j --) {
          if ((((mask << j) & data[i]) >> j) == 1) {
            frame[frameByte] |= 1;
          } else {
            frame[frameByte] &= ~1;
          }
          frameByte ++;
        }
      }*/
    };
    virtual void extract(Chunk *c, char *output, int dataBytes, int offset) {
      JBLOCKARRAY frame = (JBLOCKARRAY)c->getFrameData();
      int dataByte = 0;
      int j = 7;
      int frameByte = offset;
      for (JDIMENSION blocknum = 0; blocknum < 30; blocknum ++) {
        for (JDIMENSION i = 1; i < DCTSIZE2; i ++) {
          output[dataByte] |= ((frame[0][blocknum][i] & (1 << 2)) << j);
          j --;
          if (j == -1) {
            j = 7;
            dataByte ++;
            if (dataByte == dataBytes) {
              return;
            }
          }
        }
      }
      /*int i = 0;
      int j = 0;
      int frameByte = offset;
      for (i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (j = 7; j >= 0; j --) {
          output[i] |= ((frame[frameByte] & 1) << j);
          frameByte ++;
        }
      }*/
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'D', 'C', 'T'};
      memcpy(out, tmp, 4);
    };
};
