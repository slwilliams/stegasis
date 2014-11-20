#include <stdio.h>
#include <string.h>
#include <string>
#include "crypto/pwdbased.h"
#include "crypto/cryptlib.h"
#include "crypto/randpool.h"
#include "crypto/whrlpool.h"
extern "C" {
//  #include "libjpeg/jpeglib.h"
//  #include "libjpeg/transupp.h"  
}

#include "steganographic_algorithm.h"

class PDCTAlgorithm : public SteganographicAlgorithm {
  private:
    char *key;
    LCG lcg;

  public:
    PDCTAlgorithm(std::string password, VideoDecoder *dec) {
      printf("hello\n");
      this->password = password;
      this->dec = dec;
      this->key = (char *)malloc(128 * sizeof(char));
      char salt[16];
      int fileSize = 100;
      int numFrames = 100;
      int frameSize = this->dec->frameSize();
      int other = 0;
      printf("cool123 fils: %d, numF: %d, frames: %d, other: %d\n", fileSize,  numFrames, frameSize, other);
      memcpy(salt, &fileSize, 4);
      memcpy(salt + 4, &numFrames, 4);
      memcpy(salt + 8, &frameSize, 4);
      memcpy(salt + 12, &other, 4);
      printf("memcpy?\n");
      CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::Whirlpool> keyDeriver;
      printf("memcpy1, len: %d, pass: %s\n", this->password.length(), this->password.c_str());
      for (int i = 0; i < 16; i ++) {
        salt[i] = 0;
      }   
      keyDeriver.DeriveKey((unsigned char *)this->key, 128, 0,
          (const unsigned char *)this->password.c_str(), this->password.length(),
          (const unsigned char *)salt, 16, 32, 0);
      printf("memcpy2?\n");
      int lcgKey = key[0] | key[1] << 8 | key[2] << 16 | key[3] << 24;
      printf("memcpy3?\n");
      if (lcgKey < 0) lcgKey *= -1;
      printf("cool\n");
      this->lcg = LCG(frameSize, lcgKey, true);
      printf("finished here?\n");
    };
    void getCoef(int frameByte, int *row, int *block, int *co) {
      *row = frameByte / (64 * 80);
      *block = (frameByte - *row * 80 * 64) / 64;
      if (*block < 0) *block = 0;
      *co = frameByte % 64;
    };
    virtual void embed(Chunk *c, char *data, int dataBytes, int offset) {
      printf("embed: offset: %d\n", offset);
      int frameByte = lcg.map[offset++];
      int row, block, co;
      JBLOCKARRAY frame;
      for (int i = 0; i < dataBytes; i ++) {
        for (int j = 7; i >= 0; j --) {
          this->getCoef(frameByte, &row, &block, &co);
          printf("embed: row: %d, block: %d, co: %d\n", row, block, co);
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);
          if ((((1 << j) & data[i]) >> j) == 1) {
            frame[0][block][co] |= 1;
          } else {
            frame[0][block][co] &= ~1;
          }
          frameByte = lcg.map[offset++];
        }
      }
    };
    virtual void extract(Chunk *c, char *output, int dataBytes, int offset) {
      int frameByte = lcg.map[offset++];
      int row, block, co;
      JBLOCKARRAY frame;
      for (int i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (int j = 7; j >= 0; j --) {
          this->getCoef(frameByte, &row, &block, &co);
          frame = (JBLOCKARRAY)c->getFrameData(row, 1);
          output[i] |= ((frame[0][block][co] & 1) << j); 
          frameByte = lcg.map[offset++];
        }
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'P', 'D', 'C', 'T'};
      memcpy(out, tmp, 4);
    };
};
