#include <stdio.h>
#include <string.h>
#include <string>

#include <crypto/pwdbased.h>
#include <crypto/cryptlib.h>
#include <crypto/randpool.h>
#include <crypto/whrlpool.h>
#include <crypto/aes.h>
#include <crypto/serpent.h>
#include <crypto/twofish.h>
#include <crypto/modes.h>

#include "video/video_decoder.h"
#include "steganographic_algorithm.h"
#include "lcg.h"

using namespace std;

class DCT3Algorithm : public SteganographicAlgorithm {
  private:
    char *key;
    LCG lcg;

  public:
    DCT3Algorithm(string password, VideoDecoder *dec) {
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
          (const unsigned char *)salt, 16, 32, 0);
      int lcgKey = key[0] | key[1] << 8 | key[2] << 16 | key[3] << 24;
      if (lcgKey < 0) lcgKey *= -1;
      this->lcg = LCG(frameSize+(dec->getFrameWidth() * dec->getFrameHeight()), lcgKey, true);
    };
    void getCoef(int frameByte, int *row, int *block, int *co) {
      *row = frameByte / (DCTSIZE2 * this->dec->getFrameWidth());
      *block = (frameByte - *row * this->dec->getFrameWidth() * DCTSIZE2) / DCTSIZE2;
      if (*block < 0) *block = 0;
      *co = frameByte % DCTSIZE2;
    };
    virtual void embed(Frame *c, char *data, int dataBytes, int offset) {
      CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption cfbAesEncryption((unsigned char *)key,
         CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbAesEncryption.ProcessData((unsigned char *)data, (byte*)data, dataBytes);
      CryptoPP::CFB_Mode<CryptoPP::Twofish>::Encryption cfbTwofishEncryption((unsigned char *)key,
         CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbTwofishEncryption.ProcessData((unsigned char *)data, (byte*)data, dataBytes);
      CryptoPP::CFB_Mode<CryptoPP::Serpent>::Encryption cfbSerpentEncryption((unsigned char *)key,
         CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbSerpentEncryption.ProcessData((unsigned char *)data, (byte*)data, dataBytes);

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
    virtual void extract(Frame *c, char *output, int dataBytes, int offset) {
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

      CryptoPP::CFB_Mode<CryptoPP::Serpent>::Decryption cfbSerpentDecryption((unsigned char *)key,
         CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbSerpentDecryption.ProcessData((unsigned char *)output, (byte*)output, dataBytes);
      CryptoPP::CFB_Mode<CryptoPP::Twofish>::Decryption cfbTwofishDecryption((unsigned char *)key,
         CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbTwofishDecryption.ProcessData((unsigned char *)output, (byte*)output, dataBytes);
      CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption cfbAesDecryption((unsigned char *)key,
         CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbAesDecryption.ProcessData((unsigned char *)output, (byte*)output, dataBytes);
    };
    virtual void getAlgorithmCode(char out[4]) {
      char tmp[4] = {'D', 'C', 'T', '3'};
      memcpy(out, tmp, 4);
    };
};
