#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>

#include "crypt/cryptographic_algorithm.h"
#include "steg/steganographic_algorithm.h"
#include "video/video_decoder.h"

using namespace std;

class DCTPAlgorithm : public SteganographicAlgorithm {
  public:
    DCTPAlgorithm(string password, VideoDecoder *dec, CryptographicAlgorithm *crypt): SteganographicAlgorithm(password, dec, crypt) {};
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) {
      this->crypt->encrypt(data, reqByteCount);

      int frameByte, bytesEmbedded = 0, originalOffset = offset;
      int row, block, co;
      JBLOCKARRAY frame;
      while (bytesEmbedded < reqByteCount && offset < this->dec->getFrameSize()) {
        for (int j = 7; j >= 0; j --) {
          if (offset == this->dec->getFrameSize()) {
            bytesEmbedded --;
            break;
          }
          frameByte = lcg.map[offset++];
          this->getCoef(frameByte, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, (co % 2) + 1);
          if ((((1 << j) & data[bytesEmbedded]) >> j) == 1) {
            frame[0][block][co] |= 1;
          } else {
            frame[0][block][co] &= ~1;
          }
        }
        bytesEmbedded ++;
      }
      int currentFrame, currentFrameOffset;
      this->dec->getNextFrameOffset(&currentFrame, &currentFrameOffset);

      if (offset == this->dec->getFrameSize()) {
        this->dec->setNextFrameOffset(currentFrame + 1, 0);
      } else {
        this->dec->setNextFrameOffset(currentFrame, currentFrameOffset + (offset - originalOffset));
      }

      this->crypt->decrypt(data, reqByteCount);
      return bytesEmbedded;
    };
    virtual pair<int, int> extract(Frame *c, char *output, int dataBytes, int offset) {
      int frameByte, row, block, co;
      JBLOCKARRAY frame;
      for (int i = 0; i < dataBytes; i ++) {
        int tmp = 0;
        for (int j = 7; j >= 0; j --) {
          if (offset == this->dec->getFrameSize()) {
            this->crypt->decrypt(output, i);
            return make_pair(i, 0);
          }
          frameByte = lcg.map[offset++];
          this->getCoef(frameByte, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, (co % 2) + 1);
          tmp |= ((frame[0][block][co] & 1) << j); 
        }
        output[i] = tmp;
      }
      this->crypt->decrypt(output, dataBytes);
      return make_pair(dataBytes, offset);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'D', 'C', 'T', 'P'};
      memcpy(out, tmp, 4);
    };
};
