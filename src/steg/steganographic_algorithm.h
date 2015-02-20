#ifndef stegalg_h
#define stegalg_h 

#include <string>
#include <utility>
#include <string.h>

#include <crypto/pwdbased.h>
#include <crypto/whrlpool.h>

#include "steg/lcg.h"
#include "crypt/cryptographic_algorithm.h"
#include "video/video_decoder.h"

#define DCTSIZE2 64

using namespace std;

class SteganographicAlgorithm {                                                 
  protected:
    VideoDecoder *dec;
    CryptographicAlgorithm *crypt;
    char *key;
    LCG lcg;
    string password;

    void getCoef(int frameByte, int *row, int *block, int *co) {
      *row = frameByte / (DCTSIZE2 * this->dec->getFrameWidth());
      *block = (frameByte - *row * this->dec->getFrameWidth() * DCTSIZE2) / DCTSIZE2;
      if (*block < 0) *block = 0;
      *co = frameByte % DCTSIZE2;
    };

  public:                                                                       
    SteganographicAlgorithm(string password, VideoDecoder *dec, CryptographicAlgorithm *crypt): password(password), dec(dec), crypt(crypt) {
      this->key = (char *)malloc(128 * sizeof(char));
      char salt[16];
      int numFrames = this->dec->getNumberOfFrames();
      int frameSize = this->dec->getFrameSize();
      int width = this->dec->getFrameWidth();
      int height = this->dec->getFrameHeight();
      printf("numFrames: %d\n", numFrames);
      printf("frameSize: %d\n", frameSize);
      printf("width: %d\n", width);
      printf("height: %d\n", height);
      memcpy(salt, &numFrames, 4);
      memcpy(salt + 4, &frameSize, 4);
      memcpy(salt + 8, &width, 4);
      memcpy(salt + 12, &height, 4);
      CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::Whirlpool> keyDeriver;
      keyDeriver.DeriveKey((unsigned char *)this->key, 128, 0,
          (const unsigned char *)this->password.c_str(), this->password.length(),
          (const unsigned char *)salt, 16, 32, 0);
      int lcgKey = this->key[0] | this->key[1] << 8 | this->key[2] << 16 | this->key[3] << 24;
      if (lcgKey < 0) lcgKey *= -1;
      this->lcg = LCG(dec->getFrameSize(), lcgKey);
    };
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) = 0;
    virtual pair<int,int> extract(Frame *c, char *output, int reqByteCount, int offset) = 0;
    virtual void getAlgorithmCode(char out[4]) = 0;
};
#endif
