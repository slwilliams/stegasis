#include <stdio.h>
#include <string.h>
#include <string>
#include <pwdbased.h>
#include <cryptlib.h>
#include <randpool.h>
#include <whrlpool.h>

#include "steganographic_algorithm.h"
#include "lcg.h"

class LSBPAlgorithm : public SteganographicAlgorithm {
  private:
    char *key;
    LCG lcg;

  public:
    LSBPAlgorithm(std::string password, VideoDecoder *dec) {
      this->password = password;
      this->dec = dec;
      this->key = (char *)malloc(128 * sizeof(char));
      char salt[16];
      int fileSize = this->dec->getFileSize();
      int numFrames = this->dec->numberOfFrames();
      int height = this->dec->frameHeight();
      int width = this->dec->frameWidth();
      memcpy(salt, &fileSize, 4); 
      memcpy(salt + 4, &numFrames, 4); 
      memcpy(salt + 8, &height, 4); 
      memcpy(salt + 12, &width, 4); 
      CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::Whirlpool> keyDeriver;
      keyDeriver.DeriveKey((unsigned char *)this->key, 128, 0,
          (const unsigned char *)this->password.c_str(), this->password.length(),
          (const unsigned char *)salt, 16, 32, 0) ;
      int lcgKey = key[0] | key[1] << 8 | key[2] << 16 | key[3] << 24;
      if (lcgKey < 0) lcgKey *= -1;
      this->lcg = LCG(this->dec->frameSize(), lcgKey);
    };
    virtual void embed(char *frame, char *data, int dataBytes, int offset) {
      int i = 0;
      int j = 0;
      int frameByte = lcg.map[offset++]; 
      for (i = 0; i < dataBytes; i ++) {
        for (j = 7; j >= 0; j --) {
          if ((((1 << j) & data[i]) >> j) == 1) {
            frame[frameByte] |= 1;
          } else {
            frame[frameByte] &= ~1;
          }
          frameByte = lcg.map[offset++];
        }
      }
    };
    virtual void extract(char *frame, char *output, int dataBytes, int offset) {
      /*LCG myLCG = this->lcg.getLCG();
      // Set the seed using the 'global' lcg map
      myLCG.setSeed(lcg.map[offset]);*/

      int i = 0;
      int j = 0;
      //int frameByte = myLCG.iterate();
      int frameByte = lcg.map[offset++]; 
      for (i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (j = 7; j >= 0; j --) {
          output[i] |= ((frame[frameByte] & 1) << j);
          //frameByte = myLCG.iterate();
          frameByte = lcg.map[offset++];
        }
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', 'P'};
      memcpy(out, tmp, 4);
    };
};
