#include <stdio.h>
#include <string.h>

#include "steganographic_algorithm.h"

class LSBAlgorithm : public SteganographicAlgorithm {
  public:
    virtual void embed(Frame *c, char *data, int dataBytes, int offset) {
      char *frame = c->getFrameData();
      for (int i = 0; i < dataBytes; i ++) {
        for (int j = 7; j >= 0; j --) {
          if ((((1 << j) & data[i]) >> j) == 1) {
            frame[offset++] |= 1;
          } else {
            frame[offset++] &= ~1;
          }
        }
      }
    };
    virtual void extract(Frame *c, char *output, int dataBytes, int offset) {
      char *frame = c->getFrameData();
      for (int i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (int j = 7; j >= 0; j --) {
          output[i] |= ((frame[offset++] & 1) << j);
        }
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', ' '};
      memcpy(out, tmp, 4);
    };
};
