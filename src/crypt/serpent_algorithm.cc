#include <stdio.h>
#include <string>

#include <crypto/serpent.h>
#include <crypto/modes.h>

#include "crypt/cryptographic_algorithm.h"

using namespace std;

class SerpentAlgorithm : public CryptographicAlgorithm {
  public:
    SerpentAlgorithm(string password, VideoDecoder *dec): CryptographicAlgorithm(password, dec) {};
    virtual void encrypt(char *data, int dataBytes) {
      CryptoPP::CTR_Mode<CryptoPP::Serpent>::Encryption cfbEncryption((unsigned char *)key, 
          CryptoPP::Serpent::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbEncryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);
    };
    virtual void decrypt(char *data, int dataBytes) {
      CryptoPP::CTR_Mode<CryptoPP::Serpent>::Decryption cfbDecryption((unsigned char *)key,
          CryptoPP::Serpent::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbDecryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);
    };
};

