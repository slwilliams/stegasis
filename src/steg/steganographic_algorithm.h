#ifndef stegalg_h
#define stegalg_h 

#include <string>

#include "video/video_decoder.h"

using namespace std;

class SteganographicAlgorithm {                                                 
  protected:
    string password;
    VideoDecoder *dec;
  public:                                                                       
    virtual void embed(Frame *c, char *data, int dataBytes, int offset) = 0;
    virtual void extract(Frame *c, char *output, int dataBytes, int offset) = 0;
    virtual void getAlgorithmCode(char out[4]) = 0;
};
#endif
