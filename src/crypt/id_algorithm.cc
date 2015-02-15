#include <stdio.h>
#include <string>

#include <crypto/aes.h>
#include <crypto/modes.h>

#include "crypt/cryptographic_algorithm.h"

using namespace std;

class IDAlgorithm : public CryptographicAlgorithm {
  public:
    IDAlgorithm(string password, VideoDecoder *dec): CryptographicAlgorithm(password, dec) {};
    virtual void encrypt(char *data, int dataBytes) {};
    virtual void decrypt(char *data, int dataBytes) {};
};

