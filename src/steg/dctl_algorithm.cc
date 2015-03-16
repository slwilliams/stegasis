#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>

#include "crypt/cryptographic_algorithm.h"
#include "steg/steganographic_algorithm.h"
#include "video/video_decoder.h"

using namespace std;

class DCTLAlgorithm : public SteganographicAlgorithm {
  public:
    DCTLAlgorithm(string password, VideoDecoder *dec, CryptographicAlgorithm *crypt): SteganographicAlgorithm(password, dec, crypt) {};
    virtual pair<int, int> embed(Frame *c, char *data, int reqByteCount, int offset) {
      this->crypt->encrypt(data, reqByteCount);

      int originalOffset = offset, bytesEmbedded = 0;
      int row, block, co;
      JBLOCKARRAY frame;
      while (bytesEmbedded < reqByteCount && offset < this->dec->getFrameSize()) {
        for (int j = 7; j >= 0; j --) {
          if (offset == this->dec->getFrameSize()) {
            bytesEmbedded --;
            break;
          }
          this->getCoef(offset++, &row, &block, &co);
          // TODO: Not efficient...
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);
          if ((((1 << j) & data[bytesEmbedded]) >> j) == 1) {
            frame[0][block][co] |= 1;
          } else {
            frame[0][block][co] &= ~1;
          }
        }
        bytesEmbedded ++;
      }
      //int currentFrame, currentFrameOffset;
      //this->dec->getNextFrameOffset(&currentFrame, &currentFrameOffset);

      if (offset == this->dec->getFrameSize()) {
        //this->dec->setNextFrameOffset(currentFrame + 1, 0);
        this->crypt->decrypt(data, reqByteCount);
        return make_pair(bytesEmbedded, 0);
      } else {
        //this->dec->setNextFrameOffset(currentFrame, currentFrameOffset + (offset - originalOffset));
        this->crypt->decrypt(data, reqByteCount);
        return make_pair(bytesEmbedded, offset);
      }
    };
    virtual pair<int, int> extract(Frame *c, char *output, int dataBytes, int offset) {
      int row, block, co;
      JBLOCKARRAY frame;
      for (int i = 0; i < dataBytes; i ++) {
        int tmp = 0;
        for (int j = 7; j >= 0; j --) {
          if (offset == this->dec->getFrameSize()) {
            this->crypt->decrypt(output, i);
            return make_pair(i, 0);
          }
          this->getCoef(offset++, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);
          tmp |= ((frame[0][block][co] & 1) << j);            
        }
        output[i] = tmp;
      }
      this->crypt->decrypt(output, dataBytes);
      return make_pair(dataBytes, offset);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'D', 'C', 'T', 'L'};
      memcpy(out, tmp, 4);
    };
};
