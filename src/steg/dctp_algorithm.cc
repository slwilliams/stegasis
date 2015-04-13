#include <stdio.h>
#include <string.h>
#include <string>
#include <utility>

#include <libjpeg/jpeglib.h>

#include "crypt/cryptographic_algorithm.h"
#include "steg/steganographic_algorithm.h"
#include "video/video_decoder.h"

using namespace std;

class DCTPAlgorithm : public SteganographicAlgorithm {
  public:
    DCTPAlgorithm(string pass, VideoDecoder *dec, CryptographicAlgorithm *crypt): SteganographicAlgorithm(pass, dec, crypt) {};
    virtual pair<int, int> embed(Frame *c, char *data, int reqByteCount, int offset) {
      this->crypt->encrypt(data, reqByteCount);

      int bytesEmbedded = 0;
      int frameByte, row, block, co;
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
      this->crypt->decrypt(data, reqByteCount);
      if (offset == this->dec->getFrameSize()) {
        return make_pair(bytesEmbedded, 0);  
      } else {
        return make_pair(bytesEmbedded, offset);
      }
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
