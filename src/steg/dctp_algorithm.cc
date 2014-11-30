#include <stdio.h>
#include <string.h>
#include <string>

#include <crypto/pwdbased.h>
#include <crypto/cryptlib.h>
#include <crypto/randpool.h>
#include <crypto/whrlpool.h>

#include "steganographic_algorithm.h"
#include "lcg.h"

class DCTPAlgorithm : public SteganographicAlgorithm {
  private:
    char *key;
    LCG lcg;

  public:
    DCTPAlgorithm(std::string password, VideoDecoder *dec) {
      this->password = password;
      this->dec = dec;
      this->key = (char *)malloc(128 * sizeof(char));
      char salt[16];
      int numFrames = this->dec->numberOfFrames();
      int frameSize = this->dec->frameSize();
      int width = this->dec->frameWidth();
      int height = this->dec->frameHeight();
      memcpy(salt, &height, 4);
      memcpy(salt + 4, &numFrames, 4);
      memcpy(salt + 8, &frameSize, 4);
      memcpy(salt + 12, &width, 4);
      CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::Whirlpool> keyDeriver;
      keyDeriver.DeriveKey((unsigned char *)this->key, 128, 0,
          (const unsigned char *)this->password.c_str(), this->password.length(),
          (const unsigned char *)salt, 16, 32, 0);
      int lcgKey = key[0] | key[1] << 8 | key[2] << 16 | key[3] << 24;
      if (lcgKey < 0) lcgKey *= -1;
      this->lcg = LCG(frameSize+(dec->frameWidth() * dec->frameHeight()), lcgKey, true);
    };
    void getCoef(int frameByte, int *row, int *block, int *co) {
      //height is num of rows, width is the 
      //width is blocks per row
      *row = frameByte / (DCTSIZE2 * this->dec->frameWidth());
      *block = (frameByte - *row * this->dec->frameWidth() * DCTSIZE2) / DCTSIZE2;
      if (*block < 0) *block = 0;
      *co = frameByte % DCTSIZE2;
    };
    virtual void embed(Chunk *c, char *data, int dataBytes, int offset) {
      int frameByte = lcg.map[offset++];
      int row, block, co, comp;
      JBLOCKARRAY frame;
      for (int i = 0; i < dataBytes; i ++) {
        for (int j = 7; j >= 0; j --) {
          this->getCoef(frameByte, &row, &block, &co);
          comp = (co % 2) + 1;
          frame = (JBLOCKARRAY)c->getFrameData(row, comp);
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
      int row, block, co, comp;
      JBLOCKARRAY frame;
      for (int i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (int j = 7; j >= 0; j --) {
          this->getCoef(frameByte, &row, &block, &co);
          comp = (co % 2) + 1;
          frame = (JBLOCKARRAY)c->getFrameData(row, comp);
          output[i] |= ((frame[0][block][co] & 1) << j); 
          frameByte = lcg.map[offset++];
        }
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'D', 'C', 'T', 'P'};
      memcpy(out, tmp, 4);
    };
};
