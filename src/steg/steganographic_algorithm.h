#ifndef stegalg_h
#define stegalg_h 

#include <string>

class SteganographicAlgorithm {                                                 
  protected:
    std::string key;
  public:                                                                       
    virtual void embed(char *frame, char *data, int dataBytes, int offset) = 0;
    virtual void extract(char *frame, char *output, int dataBytes, int offset) = 0;                                 
    virtual void getAlgorithmCode(char out[4]) = 0;
};
#endif
