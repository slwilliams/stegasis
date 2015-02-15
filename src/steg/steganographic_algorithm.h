#ifndef stegalg_h
#define stegalg_h 

#include <string>
#include <utility>
#include <string.h>

#include <crypto/pwdbased.h>
#include <crypto/whrlpool.h>

#include "steg/lcg.h"
#include "video/video_decoder.h"

using namespace std;

class SteganographicAlgorithm {                                                 
  protected:
    VideoDecoder *dec;
    char *key;
    LCG lcg;
    string password;

  public:                                                                       
    SteganographicAlgorithm(string password, VideoDecoder *dec): password(password), dec(dec) {
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
    };
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) = 0;
    virtual pair<int,int> extract(Frame *c, char *output, int reqByteCount, int offset) = 0;
    virtual void getAlgorithmCode(char out[4]) = 0;
};
#endif
