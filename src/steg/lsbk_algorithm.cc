#include <stdio.h>
#include <string.h>
#include <string>
#include <pwdbased.h>
#include <cryptlib.h>

#include "steganographic_algorithm.h"

class LSBKAlgorithm : public SteganographicAlgorithm {
  public:
    LSBKAlgorithm(std::string key) {this->key = key;};
    virtual void embed(char *frame, char *data, int dataBytes, int offset) {
      CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::MessageAuthenticationCode>;

      int i = 0;
      int j = 0;
      int frameByte = offset;
      char mask = 1;
      for (i = 0; i < dataBytes; i ++) {
        for (j = 7; j >= 0; j --) {
          if ((((mask << j) & data[i]) >> j) == 1) {
            frame[frameByte] |= 1;
          } else {
            frame[frameByte] &= ~1;
          }
          frameByte ++;
        }
      }
    };
    virtual void extract(char *frame, char *output, int dataBytes, int offset) {
      int i = 0;
      int j = 0;
      int frameByte = offset;
      for (i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (j = 7; j >= 0; j --) {
          output[i] |= ((frame[frameByte] & 1) << j);
          frameByte ++;
        }
      }
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', 'K'};
      memcpy(out, tmp, 4);
    };
};
