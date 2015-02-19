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
    int hash(int *coefficients, int count) {
      int out = 0;
      for (int i = 1; i <= count; i ++) {
        out ^= ((abs(coefficients[i-1]) % 2) * i);
      }
      return out;      
    };
    int *getNextBlock(Frame *c, int offset, int blockSize) {
      int *nextBlock = (int *)malloc(sizeof(int) * blockSize);
      int row, block, co;
      for (int i = 0; i < blockSize; i ++) {
        if (offset == this->dec->getFrameHeight() * this->dec->getFrameWidth() * 64) {
          printf("offset == framesize\n");
          return NULL;
        }
        this->getCoef(lcg.map[offset++], &row, &block, &co);
        JBLOCKARRAY frame = (JBLOCKARRAY)c->getFrameData(row, 1); 
        if (frame[0][block][co] == 0) {
          i --;
          continue;
        }
        nextBlock[i] = frame[0][block][co];
      }
      /*for (int i = 0; i < blockSize; i ++) {
        printf("%d, ", nextBlock[i]);
      }
      printf("\n");*/
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
            printf("adding\n");
            frame[0][block][co] ++;
          }
          else {
            printf("deccing: %d\n", frame[0][block][co]);
            frame[0][block][co] --;
          }

          if (frame[0][block][co] == 0) return 0;
          return 1;
        }
      }
      printf("shouldn't get here\n");
      abort();
    };
    int getNextDataBlock(char *data, int dataSize, int blockSizeInBits, int offsetInBits) {
      printf("datasize: %d, blocksizebits: %d, offsetbits: %d\n", dataSize, blockSizeInBits, offsetInBits);
      printf("data[0]: %d\n", data[0]);
      int output = 0;
      for (int i = 0; i < blockSizeInBits; i ++) {
        int byte = offsetInBits / 8;
        int bit = offsetInBits % 8;
        if (byte == dataSize) return output;
        char theByte = data[byte];
        int theBit = theByte & (1 << (8*byte + 7-bit));
        output |= theBit; 

        offsetInBits ++;
      }
      printf("output: %d\n", output);
      return output;
    };
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) {
      this->crypt->encrypt(data, reqByteCount);

      if (offset != 0) {
        printf("Offset != 0!!, probably not good...\n");
        abort();
      }

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
        k ++;
      }
      k --;
      printf("K: %d\n", k);

      int codeWordLength = pow(2, k) - 1;

      int bitsEmbedded = 0;
      while (bitsEmbedded < bitsToEmbed) {
        int *coefficients = this->getNextBlock(c, offset, codeWordLength);
        if (coefficients == NULL) break;

        int hashOfCoefficients = this->hash(coefficients, codeWordLength);
        printf("hash of co: %d\n", hashOfCoefficients);
        int dataBlock = this->getNextDataBlock(data, reqByteCount-bitsEmbedded/8, k, bitsEmbedded);

        int index = hashOfCoefficients ^ dataBlock;
        printf("index: %d\n", index);
        if (index == 0) {
          // Don't need to do anything
          bitsEmbedded += k;
        // NOT true could be larger than code word length...
          offset += codeWordLength;
          continue;
        } else {
          // Need to decrement coefficent at index
          index --;
          // Won't work what if this is a zero co...
 
          if (this->decCo(c, offset, codeWordLength, index) == 0) {
            // Shrinkage...
            printf("Srinkage...\n");
            continue;
          } else {
            // ---------- tmp --------
            int *tmp = this->getNextBlock(c, offset, codeWordLength);
            int tmph = this->hash(tmp, codeWordLength);
            printf("tmph: %d\n", tmph);
            if ((tmph ^ this->getNextDataBlock(data, reqByteCount-bitsEmbedded/8, k, bitsEmbedded)) != 0) {
              printf("BAD!\n");
              abort();
            }
            printf("Worked ok!\n");
            // ----------- end tmp -----------

            bitsEmbedded += k;
        // NOT true could be larger than code word length...
            offset += codeWordLength;
            continue;
          }
        }
      }
      int currentFrame, currentFrameOffset;
      this->dec->getNextFrameOffset(&currentFrame, &currentFrameOffset);
      this->dec->setNextFrameOffset(currentFrame + 1, 0);

      this->crypt->decrypt(data, reqByteCount);
      return bitsEmbedded / 8;
    }; 
    virtual pair<int, int> extract(Frame *c, char *output, int dataBytes, int offset) {
      if (offset != 0) {
        printf("Offset != 0!!, probably not good...\n");
        abort();
      }

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
      printf("total: %d, one: %d, zero: %d\n", totalCoefficients, oneCoefficients, zeroCoefficients);

      // In bits
      int embeddingCapacity = totalCoefficients - totalCoefficients/64 - zeroCoefficients - oneCoefficients - 0.49*oneCoefficients;
      printf("Embedding capacity: %d\n", embeddingCapacity);

      int bitsToEmbed = min(dataBytes * 8, (int)(embeddingCapacity * (this->dec->getCapacity()/100.0)));
      printf("bitsToEmbed: %d\n", bitsToEmbed);

      double embeddingRate = (double)bitsToEmbed / (double)embeddingCapacity; 
      printf("EmbeddingRate: %f\n", embeddingRate);

      int k = 1;
      while (true) {
        double rate = (double)k / (pow(2, k) - 1);
        if (rate < embeddingRate) break;
        k ++;
      }
      k --;
      printf("K: %d\n", k);

      int codeWordLength = pow(2, k) - 1;

      int bitsExtracted = 0, bytesExtracted = 0;
      char temp = 0;
      char tempBit = 7;
      while (bitsExtracted < dataBytes * 8) {
        int *coefficients = this->getNextBlock(c, offset, codeWordLength);
        // NOT true could be larger than code word length...
        offset += codeWordLength;
        if (coefficients == NULL) {
          printf("here??\n");
          this->crypt->decrypt(output, bytesExtracted);   
          return make_pair(bytesExtracted, 0);
        } 

        int hashOfCoefficients = this->hash(coefficients, codeWordLength);
        printf("hasofco: %d\n", hashOfCoefficients);
        for (int i = k-1; i >= 0; i --) {
          temp |= ((hashOfCoefficients & (1 << i)) >> i) << tempBit;
          bitsExtracted ++;
          tempBit --;

          if (tempBit == 0) {
            tempBit = 7;
            printf("about to assign temp: %d\n", temp);
            output[bytesExtracted++] = temp;
            temp = 0;
          } else {
            tempBit --;
          }
        }
      }
      if (temp != 0) {
        printf("Temp should be 0...\n\n");
        abort();
      }
      printf("retting: %d\n", dataBytes);
      this->crypt->decrypt(output, dataBytes);
      return make_pair(dataBytes, offset - 1);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'F', '4', ' ', ' '};
      memcpy(out, tmp, 4);
    };
};
