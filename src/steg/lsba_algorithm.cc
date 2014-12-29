#include <stdio.h>
#include <string.h>
#include <string>

#include <crypto/pwdbased.h>
#include <crypto/cryptlib.h>
#include <crypto/randpool.h>
#include <crypto/whrlpool.h>
#include <crypto/aes.h>
#include <crypto/modes.h>


#include "steganographic_algorithm.h"
#include "lcg.h"

class LSBAAlgorithm : public SteganographicAlgorithm {
  private:
    char *key;
    LCG lcg;

  public:
    LSBAAlgorithm(std::string password, VideoDecoder *dec) {
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
    virtual void embed(Chunk *c, char *data, int dataBytes, int offset) {
      char *frame = c->getFrameData(0);

      CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption cfbEncryption((unsigned char *)key, 
          CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbEncryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);

      int frameByte = lcg.map[offset++];
      for (int i = 0; i < dataBytes; i ++) {
        for (int j = 7; j >= 0; j --) {
          if ((((1 << j) & data[i]) >> j) == 1) {
            frame[frameByte] |= 1;
          } else {
            frame[frameByte] &= ~1;
          }
          frameByte = lcg.map[offset++];
        }
      }
    };
    virtual void extract(Chunk *c, char *output, int dataBytes, int offset) {
      char *frame = c->getFrameData(0);

      int frameByte = lcg.map[offset++];
      for (int i = 0; i < dataBytes; i ++) {
        output[i] = 0;
        for (int j = 7; j >= 0; j --) {
          output[i] |= ((frame[frameByte] & 1) << j);
          frameByte = lcg.map[offset++];
        }
      }

      CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption cfbDecryption((unsigned char *)key,
          CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbDecryption.ProcessData((unsigned char *)output, (unsigned char *)output, dataBytes);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'L', 'S', 'B', '2'};
      memcpy(out, tmp, 4);
    };
};
