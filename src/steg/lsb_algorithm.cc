#include <stdio.h>

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
          frame[frameByte] |= ((mask << j) & data[i]) >> j;
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
};
