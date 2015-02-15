#include <stdio.h>
#include <string>

#include <crypto/aes.h>
#include <crypto/modes.h>

#include "crypt/cryptographic_algorithm.h"

using namespace std;

class AESAlgorithm : public CryptographicAlgorithm {
  public:
    AESAlgorithm(string password, VideoDecoder *dec): CryptographicAlgorithm(password, dec) {};
    virtual void encrypt(char *data, int dataBytes) {
      CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption cfbEncryption((unsigned char *)key, 
          CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbEncryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);
    };
    virtual void decrypt(char *data, int dataBytes) {
      CryptoPP::CFB_Mode<CryptoPP::AES>::Decryption cfbDecryption((unsigned char *)key,
          CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      cfbDecryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);
    };
};

