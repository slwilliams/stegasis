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
  private:
    char *key;
    LCG lcg;

  public:
    F4Algorithm(string password, VideoDecoder *dec) {
      this->password = password;
      this->dec = dec;
      this->key = (char *)malloc(128 * sizeof(char));
      char salt[16];
      int numFrames = this->dec->getNumberOfFrames();
      int frameSize = this->dec->getFrameSize();
      int width = this->dec->getFrameWidth();
      int height = this->dec->getFrameHeight();
      memcpy(salt, &numFrames, 4);
      memcpy(salt + 4, &frameSize, 4);
      memcpy(salt + 8, &width, 4);
      memcpy(salt + 12, &height, 4);
      CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::Whirlpool> keyDeriver;
      keyDeriver.DeriveKey((unsigned char *)this->key, 128, 0,
          (const unsigned char *)this->password.c_str(), this->password.length(),
          (const unsigned char *)salt, 16, 32, 0);
      int lcgKey = key[0] | key[1] << 8 | key[2] << 16 | key[3] << 24;
      if (lcgKey < 0) lcgKey *= -1;
      this->lcg = LCG(frameSize+(dec->getFrameWidth() * dec->getFrameHeight()), lcgKey, true);
    };
    void getCoef(int frameByte, int *row, int *block, int *co) {
      *row = frameByte / (DCTSIZE2 * this->dec->getFrameWidth());
      *block = (frameByte - *row * this->dec->getFrameWidth() * DCTSIZE2) / DCTSIZE2;
      if (*block < 0) *block = 0;
      *co = frameByte % DCTSIZE2;
    };
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) {
      printf("------\n");
      //printf("called with offset: %d\n", offset);
      int bytesLeftInFrame = this->dec->getFrameSize() - offset;
      int bytesEmbedded = 0;
      int bitEmbedded = 0;
      int originalOffset = offset;
      int frameByte, row, block, co, comp;
      JBLOCKARRAY frame;
      
      while (bytesEmbedded * 8 < bytesLeftInFrame && bytesEmbedded < reqByteCount && offset < this->dec->getFrameSize()) {
        for (int j = 7; j >= 0; j --) {
          if (offset == this->dec->getFrameSize()) break;
          frameByte = lcg.map[offset++];
          this->getCoef(frameByte, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);

          //printf("co before: %d, &1: %d, data: %d, offset: %d\n", frame[0][block][co], frame[0][block][co] & 1, ((1 << j) & data[bytesEmbedded]) >> j, offset);
          
          // Skip 0 valued coefficients
          if (frame[0][block][co] == 0) {
            j ++;
            continue;  
          }
          printf("co: %d, offset: %d\n", frame[0][block][co], offset);

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
            j ++;  
          } else {
            // Embedded sucesfully
            bitEmbedded ++;
            if (bitEmbedded == 8) {
              bitEmbedded = 0;
              bytesEmbedded ++;
            } 
          }

          printf("co after: %d\n", frame[0][block][co]);
        }
      }
      printf("-----===---\n");
      //printf("bytesEmbedded: %d, bytesLeftInFrame: %d, reqByteCount: %d, offset: %d, this->dec->getFrameSize(): %d\n", bytesEmbedded*8, bytesLeftInFrame, reqByteCount, offset, this->dec->getFrameSize());
      int currentFrame, currentFrameOffset;
      this->dec->getNextFrameOffset(&currentFrame, &currentFrameOffset);

      if (offset == this->dec->getFrameSize()) {
        // Finished this frame
   //     printf("advancing frame\n");
        this->dec->setNextFrameOffset(currentFrame + 1, 0);
      } else {
        // Still stuff left in this frame
        printf("yo currentOff: %d, offset: %d, orig: %d\n", currentFrameOffset, offset, originalOffset);
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
      printf("______________\n");
      for (int i = 0; i < dataBytes; i ++) {
        int tmp = 0;
        for (int j = 7; j >= 0; j --) {
          this->getCoef(frameByte, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);

          //printf("co before: %d, offset: %d\n", frame[0][block][co], offset);
          
          // Skip zeros
          if (frame[0][block][co] == 0) {
            j ++;
            if (offset == this->dec->getFrameSize()) return make_pair(i,0);
            frameByte = lcg.map[offset++];
            continue;  
          }
          printf("co: %d, offset: %d\n", frame[0][block][co], offset);

          if (frame[0][block][co] < 0) {
            if ((frame[0][block][co] & 1) << j == 0) {
              //output[i] |= 1 << j;
              tmp |= 1 << j;
            }
          } else {
            if ((frame[0][block][co] & 1) << j != 0) {
              tmp |= 1 << j;
              //output[i] |= 1 << j;
            }
          }
          if (offset == this->dec->getFrameSize()) return make_pair(i,0);
          frameByte = lcg.map[offset++];
        }
        output[i] = tmp;
      }
      return make_pair(dataBytes, offset - 1);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'F', '4', ' ', ' '};
      memcpy(out, tmp, 4);
    };
};
