#include <stdio.h>
#include <string.h>
#include <string>

#include <crypto/pwdbased.h>
#include <crypto/randpool.h>
#include <crypto/whrlpool.h>

#include "video/video_decoder.h"
#include "steganographic_algorithm.h"

using namespace std;

class LSBKAlgorithm : public SteganographicAlgorithm {
  private:
    char *key;

  public:
    LSBKAlgorithm(string password, VideoDecoder *dec) {
      this->password = password;
      this->dec = dec;
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
          (const unsigned char *)salt, 16, 32, 0) ;
    };
    virtual void embed(Frame *c, char *data, int dataBytes, int offset) {
      char *frame = c->getFrameData(0);

      CryptoPP::RandomPool pool;
      pool.IncorporateEntropy((const unsigned char *)this->key, 128);
      pool.IncorporateEntropy((const unsigned char *)&offset, 4);

      int frameByte = offset;
      for (int i = 0; i < dataBytes; i ++) {
        char xord = data[i] ^ pool.GenerateByte();
        for (int j = 7; j >= 0; j --) {
          if ((((1 << j) & xord) >> j) == 1) {
            frame[frameByte] |= 1;
          } else {
            frame[frameByte] &= ~1;
          }
          frameByte ++;
        }
      }
    };
    virtual void extract(Frame *c, char *output, int dataBytes, int offset) {
      char *frame = c->getFrameData(0);

      CryptoPP::RandomPool pool;
      pool.IncorporateEntropy((const unsigned char *)this->key, 128);
      pool.IncorporateEntropy((const unsigned char *)&offset, 4);

      int frameByte = offset;
      for (int i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (int j = 7; j >= 0; j --) {
          output[i] |= ((frame[frameByte] & 1) << j);
          frameByte ++;
        }
        output[i] ^= pool.GenerateByte();
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', 'K'};
      memcpy(out, tmp, 4);
    };
};
