#ifndef stegalg_h
#define stegalg_h 

#include <string>
#include <utility>

#include "video/video_decoder.h"

using namespace std;

class SteganographicAlgorithm {                                                 
  protected:
    string password;
    VideoDecoder *dec;
  public:                                                                       
    virtual int embed(Frame *c, char *data, int reqByteCount, int offset) = 0;
    virtual pair<int,int> extract(Frame *c, char *output, int reqByteCount, int offset) = 0;
    virtual void getAlgorithmCode(char out[4]) = 0;
};
#endif
