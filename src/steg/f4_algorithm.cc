#include <stdio.h>
#include <utility>
#include <string.h>
#include <string>

#include <crypto/pwdbased.h>
#include <crypto/cryptlib.h>
#include <crypto/randpool.h>
#include <crypto/whrlpool.h>

#include "video/video_decoder.h"
#include "steganographic_algorithm.h"
#include "lcg.h"

using namespace std;

class F4Algorithm : public SteganographicAlgorithm {
  public:
    F4Algorithm(string password, VideoDecoder *dec): SteganographicAlgorithm(password, dec) {
      int lcgKey = this->key[0] | this->key[1] << 8 | this->key[2] << 16 | this->key[3] << 24;
      if (lcgKey < 0) lcgKey *= -1;
      this->lcg = LCG(dec->getFrameSize(), lcgKey);
    };
    void getCoef(int frameByte, int *row, int *block, int *co) {
      *row = frameByte / (DCTSIZE2 * this->dec->getFrameWidth());
      *block = (frameByte - *row * this->dec->getFrameWidth() * DCTSIZE2) / DCTSIZE2;
      if (*block < 0) *block = 0;
      *co = frameByte % DCTSIZE2;
    };
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) {
      int bytesEmbedded = 0;
      int bitEmbedded = 0;
      int originalOffset = offset;
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
          //printf("co after: %d\n", frame[0][block][co]);
        }
        bytesEmbedded ++;
      }
      //printf("bytesEmbedded: %d, bytesLeftInFrame: %d, reqByteCount: %d, offset: %d, this->dec->getFrameSize(): %d\n",
      //bytesEmbedded*8, bytesLeftInFrame, reqByteCount, offset, this->dec->getFrameSize());

      int currentFrame, currentFrameOffset;
      this->dec->getNextFrameOffset(&currentFrame, &currentFrameOffset);

      if (offset == this->dec->getFrameSize()) {
        // Finished this frame
        this->dec->setNextFrameOffset(currentFrame + 1, 0);
      } else {
        // Still stuff left in this frame
        //printf("yo currentOff: %d, offset: %d, orig: %d\n", currentFrameOffset, offset, originalOffset);
        //printf("offset: %d, frameSize: %d\n", offset, dec->getFrameSize());
        this->dec->setNextFrameOffset(currentFrame, currentFrameOffset + (offset - originalOffset));
      }
      return bytesEmbedded;
    }; 
    // <bytesWritten, offset>
    virtual pair<int, int> extract(Frame *c, char *output, int dataBytes, int offset) {
      //printf("extract, offset: %d\n", offset);
      int frameByte = lcg.map[offset++];
      int row, block, co, comp;
      JBLOCKARRAY frame;
      for (int i = 0; i < dataBytes; i ++) {
        int tmp = 0;
        for (int j = 7; j >= 0; j --) {
          this->getCoef(frameByte, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);

          //printf("co before: %d, offset: %d\n", frame[0][block][co], offset);
          
          // Skip zeros and DC coefficients
          if (co == 0 || frame[0][block][co] == 0) {
            j ++;
            if (offset == this->dec->getFrameSize()) return make_pair(i,0);
            frameByte = lcg.map[offset++];
            continue;  
          }
          //printf("co: %d, offset: %d\n", frame[0][block][co], offset);

          if (frame[0][block][co] < 0) {
            if ((frame[0][block][co] & 1) << j == 0) {
              tmp |= 1 << j;
            }
          } else {
            if ((frame[0][block][co] & 1) << j != 0) {
              tmp |= 1 << j;
            }
          }
          if (offset == this->dec->getFrameSize()) return make_pair(i,0);
          frameByte = lcg.map[offset++];
        }
        // Only modify output if an entire byte was extracted
        output[i] = tmp;
      }
      return make_pair(dataBytes, offset - 1);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'F', '4', ' ', ' '};
      memcpy(out, tmp, 4);
    };
};
