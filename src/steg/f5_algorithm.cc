#include <stdio.h>
#include <utility>
#include <string>

#include <libjpeg/jpeglib.h>

#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"
#include "crypt/cryptographic_algorithm.h"

#define ASSERTNEQ(cond,text) if (cond) { printf(text); abort(); }

using namespace std;

class F5Algorithm : public SteganographicAlgorithm {
  public:
    F5Algorithm(string pass, VideoDecoder *dec, CryptographicAlgorithm *crypt): SteganographicAlgorithm(pass, dec, crypt) {};
    int hash(int *coefficients, int count) {
      int out = 0;
      for (int i = 1; i <= count; i ++) {
        out ^= ((abs(coefficients[i-1]) % 2) * i);
      }
      return out;      
    };
    int *getNextCoefficientBlock(Frame *c, int *offset, int blockSize) {
      int *nextBlock = (int *)malloc(sizeof(int) * blockSize);
      int row, block, co;
      for (int i = 0; i < blockSize; i ++) {
        if (*offset == this->dec->getFrameHeight() * this->dec->getFrameWidth() * 64) {
          return NULL;
        }
        this->getCoef(lcg.map[(*offset)++], &row, &block, &co);
        JBLOCKARRAY frame = (JBLOCKARRAY)c->getFrameData(row, 1); 
        if (frame[0][block][co] == 0) {
          i --;
          continue;
        }
        nextBlock[i] = frame[0][block][co];
      }
      return nextBlock; 
    };
    int decCo(Frame *c, int offset, int codeWordLength, int index) {
      int row, block, co;
      for (int i = 0; i < codeWordLength; i ++) {
        this->getCoef(lcg.map[offset++], &row, &block, &co);
        JBLOCKARRAY frame = (JBLOCKARRAY)c->getFrameData(row, 1); 
        if (frame[0][block][co] == 0) {
          i --;
          continue;
        }
        if (i == index) {
          if (frame[0][block][co] < 1) {
            frame[0][block][co] ++;
          }
          else {
            frame[0][block][co] --;
          }

          if (frame[0][block][co] == 0) {
            // Indicate srinkage occured
            return 0;
          } else {
            // Decremented ok
            return 1;
          }
        }
      }
      ASSERTNEQ(true, "Shouldn't get here...\n");
    };
    int getNextDataBlock(char *data, int dataSize, int blockSizeInBits, int offsetInBits) {
      int output = 0;
      for (int i = 0; i < blockSizeInBits; i ++) {
        int byte = offsetInBits / 8;
        int bit = offsetInBits % 8;

        if (byte == dataSize) return output;

        char theByte = data[byte];
        int theBit = (theByte & (1 << (7-bit))) >> (7-bit);
        output |= theBit << (blockSizeInBits-1 - i);

        offsetInBits ++;
      }
      return output;
    };
    virtual pair<int, int> embed(Frame *c, char *data, int reqByteCount, int offset) {
      if (reqByteCount == 0) return make_pair(0, 0);
      this->crypt->encrypt(data, reqByteCount);

      ASSERTNEQ(offset != 0, "Embed offset is not zero\n");

      JBLOCKARRAY frame;
      int row, block, co;
      
      // Estimate the embedding capacity
      int totalCoefficients = this->dec->getFrameHeight() * this->dec->getFrameWidth() * 64;
      int zeroCoefficients = 0, oneCoefficients = 0;
      for (int i = 0; i < totalCoefficients; i ++) {
        this->getCoef(lcg.map[i], &row, &block, &co); 
        frame = (JBLOCKARRAY)c->getFrameData(row, 1);
        if (frame[0][block][co] == 0) zeroCoefficients ++;
        if (frame[0][block][co] == 1 || frame[0][block][co] == -1) oneCoefficients ++;
      }
      
      // Embed value of 255 for k into the second component
      for (int j = 7; j >= 0; j --) {
        this->getCoef(lcg.map[j+5], &row, &block, &co); 
        frame = (JBLOCKARRAY)c->getFrameData(row, 2);
        if ((255 & (1 << j)) != 0) {
          frame[0][block][co] |= 1;
        } else {
          frame[0][block][co] &= (~1);
        }
      }

      // In bits
      int embeddingCapacity = totalCoefficients - totalCoefficients/64 - zeroCoefficients - oneCoefficients - 0.49*oneCoefficients;
      // Force to be a multiple of 8
      embeddingCapacity -= embeddingCapacity % 8;

      if (embeddingCapacity < 8) {
        // No point trying to embed anything in this frame
        this->crypt->decrypt(data, reqByteCount);
        return make_pair(0, 0);
      }

      int bitsToEmbed = min(reqByteCount * 8, (int)(embeddingCapacity * (this->dec->getCapacity()/100.0)));
      double embeddingRate = (double)bitsToEmbed / (double)embeddingCapacity; 

      char k = 1;
      while (true) {
        double rate = (double)k / (pow(2, k) - 1);
        if (rate < embeddingRate) break;
        k ++;
      }
      k --;

      // Embed value of k into the second component
      for (int j = 7; j >= 0; j --) {
        this->getCoef(lcg.map[j+5], &row, &block, &co); 
        frame = (JBLOCKARRAY)c->getFrameData(row, 2);
        if ((k & (1 << j)) != 0) {
          frame[0][block][co] |= 1;
        } else {
          frame[0][block][co] &= (~1);
        }
      }

      int codeWordLength = pow(2, k) - 1;

      int bitsEmbedded = 0;
      while (bitsEmbedded < bitsToEmbed) {
        int oldOffset = offset;

        int *coefficients = this->getNextCoefficientBlock(c, &offset, codeWordLength);
        if (coefficients == NULL) break;

        int hashOfCoefficients = this->hash(coefficients, codeWordLength);
        int dataBlock = this->getNextDataBlock(data, reqByteCount, k, bitsEmbedded);

        int index = hashOfCoefficients ^ dataBlock;
        if (index == 0) {
          // Don't need to do anything
          bitsEmbedded += k;
          continue;
        } else {
          // Need to decrement coefficent at index-1
          index --;
 
          if (this->decCo(c, oldOffset, codeWordLength, index) == 0) {
            // Shrinkage occured
            offset = oldOffset;
            continue;
          } else {
            // ---------- tmp --------
            int *tmp = this->getNextCoefficientBlock(c, &oldOffset, codeWordLength);
            int tmph = this->hash(tmp, codeWordLength);
            ASSERTNEQ((tmph ^ this->getNextDataBlock(data, reqByteCount, k, bitsEmbedded)) != 0, "Embedded data not extracted correctly.\n")
            // ----------- end tmp -----------

            bitsEmbedded += k;
            continue;
          }
        }
        free(coefficients);
      }
      
      // Embed the amount of bytes into the second comp
      short toEmbed = (short)(bitsEmbedded / 8);
      for (int j = 15; j >= 0; j --) {
        this->getCoef(lcg.map[j+15], &row, &block, &co); 
        frame = (JBLOCKARRAY)c->getFrameData(row, 2);
        if ((toEmbed & (1 << j)) != 0) {
          frame[0][block][co] |= 1;
        } else {
          frame[0][block][co] &= (~1);
        }
      }

      this->crypt->decrypt(data, reqByteCount);
      return make_pair(bitsEmbedded / 8, 0);
    }; 
    virtual pair<int, int> extract(Frame *c, char *output, int dataBytes, int offset) {
      if (dataBytes == 0) return make_pair(0, 0);
      ASSERTNEQ(offset != 0, "Extract offset not equal to 0\n")

      JBLOCKARRAY frame;
      int row, block, co;

      char k = 0;
      for (int j = 7; j >= 0; j --) {
        this->getCoef(lcg.map[j+5], &row, &block, &co); 
        frame = (JBLOCKARRAY)c->getFrameData(row, 2);
        k |= (frame[0][block][co] & 1) << j;
      }
      if (k == -1) {
        // No bytes in this frame
        return make_pair(0, 0);
      }

      int codeWordLength = pow(2, k) - 1;

      // Extract the number of bytes in this frame
      short bytesInFrame = 0;
      for (int j = 15; j >= 0; j --) {
        this->getCoef(lcg.map[j+15], &row, &block, &co); 
        frame = (JBLOCKARRAY)c->getFrameData(row, 2);
        bytesInFrame |= (frame[0][block][co] & 1) << j;
      }
      // Incorrect permutation passphrase
      if (bytesInFrame <= 0) return make_pair(dataBytes, 0);

      int bitsExtracted = 0, bytesExtracted = 0;
      char temp = 0;
      char tempBit = 7;
      while (bitsExtracted < bytesInFrame * 8) {
        int *coefficients = this->getNextCoefficientBlock(c, &offset, codeWordLength);
        if (coefficients == NULL) {
          // Hit the bottom of the frame, don't think this should ever happen
          printf("Data exceeded estimated capacity\n");
          this->crypt->decrypt(output, bytesExtracted);   
          return make_pair(bytesExtracted, 0);
        } 

        int hashOfCoefficients = this->hash(coefficients, codeWordLength);
        for (int i = k-1; i >= 0; i --) {
          temp |= (((hashOfCoefficients & (1 << i)) >> i) << tempBit);
          bitsExtracted ++;

          if (tempBit == 0) {
            tempBit = 7;
            output[bytesExtracted++] = temp;
            temp = 0;
          } else {
            tempBit --;
          }
        }
        free(coefficients);
      }
      ASSERTNEQ(temp != 0, "Temp is zero at end of extract.\n");
      //printf("Returning extracted bytes: %d\n", bytesInFrame);
      this->crypt->decrypt(output, bytesInFrame);
      return make_pair(bytesInFrame, 0);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'F', '5', ' ', ' '};
      memcpy(out, tmp, 4);
    };
};
