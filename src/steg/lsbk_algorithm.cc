#include <stdio.h>
#include <string.h>
#include <string>
#include <pwdbased.h>
#include <cryptlib.h>
#include <randpool.h>
#include <whrlpool.h>

#include "steganographic_algorithm.h"

class LSBKAlgorithm : public SteganographicAlgorithm {
  private:
    char *key;
    // TODO: make salt dependant on feature of the video...
    char salt[10] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A'};

  public:
    LSBKAlgorithm(std::string password) {
      this->password = password;
      this->key = (char *)malloc(128 * sizeof(char));
      CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::Whirlpool> keyDeriver;
      keyDeriver.DeriveKey((unsigned char *)this->key, 128, 0,
          (const unsigned char *)this->password.c_str(), this->password.length(),
          (const unsigned char *)this->salt, 10, 32, 0) ;

    };
    virtual void embed(char *frame, char *data, int dataBytes, int offset) {
      CryptoPP::RandomPool pool;
      pool.IncorporateEntropy((const unsigned char *)key, 128);

      int i = 0;
      int j = 0;
      int frameByte = offset;
      char mask = 1;
      for (i = 0; i < dataBytes; i ++) {
        char toXor = pool.GenerateByte();
        for (j = 7; j >= 0; j --) {
          if ((((mask << j) & (data[i] ^ toXor)) >> j) == 1) {
            frame[frameByte] |= 1;
          } else {
            frame[frameByte] &= ~1;
          }
          frameByte ++;
        }
      }
    };
    virtual void extract(char *frame, char *output, int dataBytes, int offset) {
      CryptoPP::RandomPool pool;
      pool.IncorporateEntropy((const unsigned char *)key, 128);

      int i = 0;
      int j = 0;
      int frameByte = offset;
      for (i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (j = 7; j >= 0; j --) {
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
