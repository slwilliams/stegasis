#include <stdio.h>
#include <string.h>

#include "steganographic_algorithm.h"

class LSBAlgorithm : public SteganographicAlgorithm {
  public:
    virtual void embed(char *frame, char *data, int dataBytes, int offset) {
      int i = 0;
      int j = 0;
      int frameByte = offset;
      char mask = 1;
      for (i = 0; i < dataBytes; i ++) {
        for (j = 7; j >= 0; j --) {
          if ((((mask << j) & data[i]) >> j) == 1) {
            printf("before: %u\n", frame[frameByte]);
            frame[frameByte] |= 1;
            printf("af: %u\n", frame[frameByte]);
          } else {
            printf("before: %u\n", frame[frameByte]);
            frame[frameByte] &= ~1;
            printf("af: %u\n", frame[frameByte]);
          }
          frameByte ++;
        }
      }
    };
    virtual void extract(char *frame, char *output, int dataBytes, int offset) {
      int i = 0;
      int j = 0;
      int frameByte = offset;
      for (i = 0; i < dataBytes; i ++) {
        for (j = 7; j >= 0; j --) {
          output[i] |= ((frame[frameByte] & 1) << j);
          frameByte ++;
        }
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', ' '};
      memcpy(out, tmp, 4);
    };
};
