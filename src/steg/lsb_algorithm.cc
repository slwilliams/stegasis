#include <utility>
#include <stdio.h>
#include <string.h>
#include <string>

#include "video/video_decoder.h"
#include "crypt/cryptographic_algorithm.h"
#include "steg/steganographic_algorithm.h"

using namespace std;

class LSBAlgorithm : public SteganographicAlgorithm {
  public:
    LSBAlgorithm(string password, VideoDecoder *dec, CryptographicAlgorithm *crypt): SteganographicAlgorithm(password, dec, crypt) {};
    virtual pair<int, int> embed(Frame *c, char *data, int reqByteCount, int offset) {
      this->crypt->encrypt(data, reqByteCount);

      char *frame = c->getFrameData();
      int bytesEmbedded = 0, frameSize = this->dec->getFrameSize();
      while (bytesEmbedded < reqByteCount && offset < frameSize) {
        for (int j = 7; j >= 0; j --) {
          frame[offset] &= ~1; 
          frame[offset++] |= ((1 << j) & data[bytesEmbedded]) >> j;  
          /*if ((((1 << j) & data[bytesEmbedded]) >> j) == 1) {
            frame[offset++] |= 1;
          } else {
            frame[offset++] &= ~1;
          }*/
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
    virtual pair<int, int> extract(Frame *c, char *output, int reqByteCount, int offset) {
      char *frame = c->getFrameData();
      for (int i = 0; i < reqByteCount; i ++) {
        int tmp = 0;
        for (int j = 7; j >= 0; j --) {
          if (offset == this->dec->getFrameSize()) {
            this->crypt->decrypt(output, i);
            return make_pair(i, 0);
          }
          tmp |= ((frame[offset++] & 1) << j);
        }
        output[i] = tmp;
      }

      this->crypt->decrypt(output, reqByteCount);
      return make_pair(reqByteCount, offset);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', ' '};
      memcpy(out, tmp, 4);
    };
};
