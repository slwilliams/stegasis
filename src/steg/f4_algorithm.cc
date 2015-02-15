#include <stdio.h>
#include <utility>
#include <string>

#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"
#include "crypt/cryptographic_algorithm.h"

using namespace std;

class F4Algorithm : public SteganographicAlgorithm {
  public:
    F4Algorithm(string password, VideoDecoder *dec, CryptographicAlgorithm *crypt): SteganographicAlgorithm(password, dec, crypt) {};
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) {
      this->crypt->encrypt(data, reqByteCount);

      int bytesEmbedded = 0, originalOffset = offset;
      int frameByte, row, block, co, comp;
      JBLOCKARRAY frame;
      while (bytesEmbedded < reqByteCount && offset < this->dec->getFrameSize()) {
        for (int j = 7; j >= 0; j --) {
          if (offset == this->dec->getFrameSize()) {
            // This byte wasn't sucsesfully embedded
            bytesEmbedded --;
            break;
          }
          frameByte = lcg.map[offset++];
          this->getCoef(frameByte, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);
          
          // Skip DC terms and 0 valued coefficients
          if (co == 0 || frame[0][block][co] == 0) {
            j ++;
            continue;  
          }

          if (frame[0][block][co] > 0) {
            if ((frame[0][block][co] & 1) != ((1 << j) & data[bytesEmbedded]) >> j) {
              frame[0][block][co] --;
            } 
          } else {
            if ((frame[0][block][co] & 1) == ((1 << j) & data[bytesEmbedded]) >> j) {
              frame[0][block][co] ++;
            }
          }
          if (frame[0][block][co] == 0) {
            // Srinkage occured, must embed bit again
            j ++;  
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
      int frameByte = lcg.map[offset++];
      int row, block, co, comp;
      JBLOCKARRAY frame;
      for (int i = 0; i < dataBytes; i ++) {
        int tmp = 0;
        for (int j = 7; j >= 0; j --) {
          this->getCoef(frameByte, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);

          // Skip zeros and DC coefficients
          if (co == 0 || frame[0][block][co] == 0) {
            j ++;
            if (offset == this->dec->getFrameSize()) {
              this->crypt->decrypt(output, i);
              return make_pair(i,0);
            }
            frameByte = lcg.map[offset++];
            continue;  
          }
          
          if (frame[0][block][co] < 0) {
            if ((frame[0][block][co] & 1) << j == 0) {
              tmp |= 1 << j;
            }
          } else {
            if ((frame[0][block][co] & 1) << j != 0) {
              tmp |= 1 << j;
            }
          }
          if (offset == this->dec->getFrameSize()) {
            this->crypt->decrypt(output, i);   
            return make_pair(i,0);
          }
          frameByte = lcg.map[offset++];
        }
        // Only modify output if an entire byte was extracted
        output[i] = tmp;
      }
      this->crypt->decrypt(output, dataBytes);
      return make_pair(dataBytes, offset - 1);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'F', '4', ' ', ' '};
      memcpy(out, tmp, 4);
    };
};
