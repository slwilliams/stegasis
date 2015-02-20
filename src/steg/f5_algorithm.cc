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
    int *getNextBlock(Frame *c, int *offset, int blockSize) {
      // TODO: memory leak
      int *nextBlock = (int *)malloc(sizeof(int) * blockSize);
      int row, block, co;
      for (int i = 0; i < blockSize; i ++) {
        if (*offset == this->dec->getFrameHeight() * this->dec->getFrameWidth() * 64) {
          printf("offset == framesize\n");
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
   //         printf("adding\n");
            frame[0][block][co] ++;
          }
          else {
    //        printf("deccing: %d\n", frame[0][block][co]);
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
   //   printf("datasize: %d, blocksizebits: %d, offsetbits: %d\n", dataSize, blockSizeInBits, offsetInBits);
    //  printf("off: %d, /8: %d, data[off/8]: %d\n", offsetInBits, offsetInBits/8, data[offsetInBits/8]);
      int output = 0;
      for (int i = 0; i < blockSizeInBits; i ++) {
        int byte = offsetInBits / 8;
        int bit = offsetInBits % 8;
        if (byte == dataSize) return output;
        char theByte = data[byte];
     //   printf("thebytes: %d\n", theByte);
        int theBit = (theByte & (1 << (7-bit))) >> (7-bit);
      //  printf("bit: %d, theBit: %d\n", bit, theBit);
        output |= theBit << (blockSizeInBits-1 - i);

        offsetInBits ++;
      }
    //  printf("output: %d\n", output);
      return output;
    };
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) {
      if (reqByteCount == 0) {
        printf("reqbyte = 0\n");
        return 0;
      }

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
      
      // Embed value of 255 into the second component
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
      printf("Embedding capacity: %d, total: %d, zero: %d, one: %d\n", embeddingCapacity, totalCoefficients, zeroCoefficients, oneCoefficients);
      if (embeddingCapacity < 8) {
        printf("embedding capcity < 8...\n");
        int currentFrame, currentFrameOffset;
        this->dec->getNextFrameOffset(&currentFrame, &currentFrameOffset);
        this->dec->setNextFrameOffset(currentFrame + 1, 0);

        this->crypt->decrypt(data, reqByteCount);
        return 0;
      }

      int bitsToEmbed = min(reqByteCount * 8, (int)(embeddingCapacity * (this->dec->getCapacity()/100.0)));
      printf("bitsToEmbed: %d, req: %d\n", bitsToEmbed, reqByteCount);

      double embeddingRate = (double)bitsToEmbed / (double)embeddingCapacity; 
      printf("EmbeddingRate: %f\n", embeddingRate);

      char k = 1;
      while (true) {
        double rate = (double)k / (pow(2, k) - 1);
        if (rate < embeddingRate) break;
        k ++;
      }
      k --;
      printf("K: %d\n", k);

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
        int *coefficients = this->getNextBlock(c, &offset, codeWordLength);
        if (coefficients == NULL) break;

        int hashOfCoefficients = this->hash(coefficients, codeWordLength);
        //printf("hash of co: %d\n", hashOfCoefficients);
        int dataBlock = this->getNextDataBlock(data, reqByteCount, k, bitsEmbedded);

        int index = hashOfCoefficients ^ dataBlock;
        //printf("index: %d\n", index);
        if (index == 0) {
          // Don't need to do anything
          bitsEmbedded += k;
          continue;
        } else {
          // Need to decrement coefficent at index
          index --;
 
          if (this->decCo(c, oldOffset, codeWordLength, index) == 0) {
            // Shrinkage...
  //       printf("Srinkage...\n");
            offset = oldOffset;
            continue;
          } else {
            // ---------- tmp --------
            int *tmp = this->getNextBlock(c, &oldOffset, codeWordLength);
            int tmph = this->hash(tmp, codeWordLength);
         //   printf("tmph: %d\n", tmph);
            if ((tmph ^ this->getNextDataBlock(data, reqByteCount, k, bitsEmbedded)) != 0) {
      //    printf("BAD!\n");
              abort();
            }
    //     printf("Worked ok!\n");
            // ----------- end tmp -----------

            bitsEmbedded += k;
        // NOT true could be larger than code word length...
            continue;
          }
        }
      }
      int currentFrame, currentFrameOffset;
      this->dec->getNextFrameOffset(&currentFrame, &currentFrameOffset);
      this->dec->setNextFrameOffset(currentFrame + 1, 0);

      this->crypt->decrypt(data, reqByteCount);
      printf("managed to embed: %d bytes\n", bitsEmbedded/8);
      return bitsEmbedded / 8;
    }; 
    virtual pair<int, int> extract(Frame *c, char *output, int dataBytes, int offset) {
      if (dataBytes == 0) return make_pair(0, 0);
      if (offset != 0) {
        printf("Offset != 0...\n");
        abort();
      }

      JBLOCKARRAY frame;
      int row, block, co;
    /* 
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
      // Force to be a multiple of 8
      embeddingCapacity -= embeddingCapacity % 8;
      printf("Embedding capacity: %d, total: %d, zero: %d, one: %d\n", embeddingCapacity, totalCoefficients, zeroCoefficients, oneCoefficients);
      if (embeddingCapacity < 8) {
        printf("embedding capcity < 8...\n");
        return make_pair(0, 0);
      }*/

      char k = 0;
      for (int j = 7; j >= 0; j --) {
        this->getCoef(lcg.map[j+5], &row, &block, &co); 
        frame = (JBLOCKARRAY)c->getFrameData(row, 2);
        k |= (frame[0][block][co] & 1) << j;
      }
      printf("K: %d\n", k);
      if (k == -1) {
        return make_pair(0, 0);
      }

      int codeWordLength = pow(2, k) - 1;

      int bitsExtracted = 0, bytesExtracted = 0;
      char temp = 0;
      char tempBit = 7;
      while (bitsExtracted < dataBytes * 8) {
        int *coefficients = this->getNextBlock(c, &offset, codeWordLength);
        if (coefficients == NULL) {
          printf("here??\n");
          this->crypt->decrypt(output, bytesExtracted);   
          return make_pair(bytesExtracted, 0);
        } 

        int hashOfCoefficients = this->hash(coefficients, codeWordLength);
       // printf("hasofco: %d\n", hashOfCoefficients);
        for (int i = k-1; i >= 0; i --) {
          temp |= (((hashOfCoefficients & (1 << i)) >> i) << tempBit);
          bitsExtracted ++;

          if (tempBit == 0) {
            tempBit = 7;
        //    printf("about to assign temp: %d\n", temp);
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
      return make_pair(dataBytes, 0);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'F', '5', ' ', ' '};
      memcpy(out, tmp, 4);
    };
};
