#include <stdio.h>
#include <string>

#include <crypto/aes.h>
#include <crypto/serpent.h>
#include <crypto/twofish.h>
#include <crypto/modes.h>

#include "crypt/cryptographic_algorithm.h"

using namespace std;

class AESSerpentTwofishAlgorithm : public CryptographicAlgorithm {
  public:
    AESSerpentTwofishAlgorithm(string password, VideoDecoder *dec): CryptographicAlgorithm(password, dec) {};
    virtual void encrypt(char *data, int dataBytes) {
      CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption aesEncryption((unsigned char *)key, 
          CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      aesEncryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);

      CryptoPP::CTR_Mode<CryptoPP::Serpent>::Encryption serpentEncryption((unsigned char *)key, 
          CryptoPP::Serpent::DEFAULT_KEYLENGTH, (unsigned char *)(key+64+16));
      serpentEncryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);

      CryptoPP::CTR_Mode<CryptoPP::Twofish>::Encryption twofishEncryption((unsigned char *)key, 
          CryptoPP::Twofish::DEFAULT_KEYLENGTH, (unsigned char *)(key+64+16+16));
      twofishEncryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);
    };
    virtual void decrypt(char *data, int dataBytes) {
      CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption aesDecryption((unsigned char *)key,
          CryptoPP::AES::DEFAULT_KEYLENGTH, (unsigned char *)(key+64));
      aesDecryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);

      CryptoPP::CTR_Mode<CryptoPP::Serpent>::Decryption serpentDecryption((unsigned char *)key,
          CryptoPP::Serpent::DEFAULT_KEYLENGTH, (unsigned char *)(key+64+16));
      serpentDecryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);

      CryptoPP::CTR_Mode<CryptoPP::Twofish>::Decryption twofishDecryption((unsigned char *)key,
          CryptoPP::Twofish::DEFAULT_KEYLENGTH, (unsigned char *)(key+64+16+16));
      twofishDecryption.ProcessData((unsigned char *)data, (unsigned char *)data, dataBytes);
    };
};

