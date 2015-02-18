#include <stdio.h>
#include <utility>
#include <string>

#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"
#include "crypt/cryptographic_algorithm.h"

#include "libjpeg/jpeglib.h"

using namespace std;

class F5Algorithm : public SteganographicAlgorithm {
  public:
    F5Algorithm(string pass, VideoDecoder *dec, CryptographicAlgorithm *crypt): SteganographicAlgorithm(pass, dec, crypt) {};
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) {
      this->crypt->encrypt(data, reqByteCount);

      if (offset != 0) {
        printf("Offset != 0!!, probably not good...\n");
        abort();
      }

      JBLOCKARRAY frame;
      int frameByte, row, block, co, comp;

      
      // Estimate the embedding capacity
      int totalCoefficients = this->dec->getFrameHeight() * this->dec->getFrameWidth() * 64;
      int zeroCoefficients = 0, oneCoefficients = 0;
      for (int i = 0; i < totalCoefficients; i ++) {
        this->getCoef(lcg.map[i], &row, &block, &co); 
        frame = (JBLOCKARRAY)c->getFrameData(row, 1);
        if (frame[0][block][co] == 0) zeroCoefficients ++;
        if (frame[0][block][co] == 1 || frame[0][block][co] == -1) oneCoefficients ++;
      }

      // In bits
      int embeddingCapacity = totalCoefficients - totalCoefficients/64 - zeroCoefficients - oneCoefficients - 0.49*oneCoefficients;
      printf("Embedding capacity: %d\n", embeddingCapacity);

      int bitsToEmbed = min(reqByteCount * 8, (int)(embeddingCapacity * (this->dec->getCapacity()/100.0)));
      printf("bitsToEMbed: %d\n", bitsToEmbed);

      double embeddingRate = (double)bitsToEmbed / (double)embeddingCapacity; 
      printf("EmbeddingRate: %f\n", embeddingRate);

      int k = 1;
      while (true) {
        double rate = (double)k / (pow(2, k) - 1);
        if (rate < embeddingRate) break;
      }
      k ++;
      printf("K: %d\n", k);

      int codeWordLength = pow(2, k) - 1;

      int bytesEmbedded = 0, originalOffset = offset;
      while (bytesEmbedded < bitsToEmbed * 8) {
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
