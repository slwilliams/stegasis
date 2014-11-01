#include <stdio.h>
#include <string.h>
#include <string>
#include <pwdbased.h>
#include <cryptlib.h>
#include <randpool.h>
#include <whrlpool.h>
#include <set>
#include <mutex> 

#include "steganographic_algorithm.h"
#include "lcg.h"

class LSBPAlgorithm : public SteganographicAlgorithm {
  private:
    char *key;
    // TODO: make salt dependant on feature of the video...
    char salt[10] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A'};
    // TODO not hard code this
    LCG lcg = LCG(2764800);
    // Need to lock embed and extract since concurrent calls to LCG are not safe
    mutex mux;

  public:
    LSBPAlgorithm(std::string password) {
      this->password = password;
      this->key = (char *)malloc(128 * sizeof(char));
      CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::Whirlpool> keyDeriver;
      keyDeriver.DeriveKey((unsigned char *)this->key, 128, 0,
          (const unsigned char *)this->password.c_str(), this->password.length(),
          (const unsigned char *)this->salt, 10, 32, 0) ;

    };
    virtual void embed(char *frame, char *data, int dataBytes, int offset) {
      this->mux.lock();
      // Reset the psudo random permutation and find the correct offset within
      // the sequence
      // Fix performance with a hasmap from offsets -> seed values
      lcg.setSeed(0);
      int i = 0;
      for (i = 0; i < offset; i ++) {
        lcg.iterate();
      }      
      
      int j = 0;
      int frameByte = lcg.iterate();
      char mask = 1;
      for (i = 0; i < dataBytes; i ++) {
        for (j = 7; j >= 0; j --) {
          if ((((mask << j) & (data[i])) >> j) == 1) {
            frame[frameByte] |= 1;
          } else {
            frame[frameByte] &= ~1;
          }
          frameByte = lcg.iterate();
        }
      }
      this->mux.unlock();
    };
    virtual void extract(char *frame, char *output, int dataBytes, int offset) {
      this->mux.lock();
      // Reset the psudo random permutation and find the correct offset within
      // the sequence
      lcg.setSeed(0);
      int i = 0;
      for (i = 0; i < offset; i ++) {
        lcg.iterate();
      }      

      int j = 0;
      int frameByte = lcg.iterate();
      char mask = 1;
      for (i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (j = 7; j >= 0; j --) {
          output[i] |= ((frame[frameByte] & 1) << j);
          frameByte = lcg.iterate();
        }
      }
      this->mux.unlock();
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', 'P'};
      memcpy(out, tmp, 4);
    };
};
