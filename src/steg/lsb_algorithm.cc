#include <stdio.h>
#include <string.h>

#include "steganographic_algorithm.h"

class LSBAlgorithm : public SteganographicAlgorithm {
  public:
    virtual void embed(Chunk *c, char *data, int dataBytes, int offset) {
      char *frame = c->getFrameData(0);
      int frameByte = offset;
      for (int i = 0; i < dataBytes; i ++) {
        for (int j = 7; j >= 0; j --) {
          if ((((1 << j) & data[i]) >> j) == 1) {
            frame[frameByte] |= 1;
          } else {
            frame[frameByte] &= ~1;
          }
          frameByte ++;
        }
      }
    };
    virtual void extract(Chunk *c, char *output, int dataBytes, int offset) {
      char *frame = c->getFrameData(0);
      int frameByte = offset;
      for (int i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (int j = 7; j >= 0; j --) {
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
