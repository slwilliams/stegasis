#ifndef cryptalg_h
#define cryptalg_h 

#include <string>
#include <utility>
#include <string.h>

#include <crypto/pwdbased.h>
#include <crypto/whrlpool.h>

#include "video/video_decoder.h"

using namespace std;

class CryptographicAlgorithm {                                                 
  protected:
    VideoDecoder *dec;
    string password;
    char *key;
  public:                            
    CryptographicAlgorithm(string password, VideoDecoder *dec): password(password), dec(dec) {
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
    virtual void encrypt(char *data, int byteCount) = 0;
    virtual void decrypt(char *data, int byteCount) = 0;
};
#endif
