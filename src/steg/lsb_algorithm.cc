#include <stdio.h>
#include <string.h>

#include "steganographic_algorithm.h"

class LSBAlgorithm : public SteganographicAlgorithm {
  public:
    LSBAlgorithm(VideoDecoder *dec) {
      this->dec = dec;
    }
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) {
      char *frame = c->getFrameData();
      int bytesLeftInFrame = this->dec->getFrameSize() - offset;
      int bytesEmbedded = 0;
      while (bytesEmbedded * 8 < bytesLeftInFrame - 8 && bytesEmbedded < reqByteCount) {
        for (int j = 7; j >= 0; j --) {
          if ((((1 << j) & data[bytesEmbedded]) >> j) == 1) {
            frame[offset++] |= 1;
          } else {
            frame[offset++] &= ~1;
          }
        }
        bytesEmbedded ++;
      }

      int currentFrame, currentFrameOffset;
      this->dec->getNextFrameOffset(&currentFrame, &currentFrameOffset);

      if (bytesEmbedded * 8 >= bytesLeftInFrame - 8) {
        // Finished this frame
        this->dec->setNextFrameOffset(currentFrame + 1, 0);
      } else {
        // Still stuff left in this frame
        this->dec->setNextFrameOffset(currentFrame, currentFrameOffset + bytesEmbedded); 
      }
      return bytesEmbedded;
    };
    virtual int extract(Frame *c, char *output, int reqByesCount, int offset) {
      char *frame = c->getFrameData();
      for (int i = 0; i < reqByesCount; i ++) {
        output[i] = 0;
        for (int j = 7; j >= 0; j --) {
          output[i] |= ((frame[offset++] & 1) << j);
        }
      }
      return reqByesCount;
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', ' '};
      memcpy(out, tmp, 4);
    };
};
