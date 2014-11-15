#ifndef stegalg_h
#define stegalg_h 

#include <string>

#include "video/video_decoder.h"

class SteganographicAlgorithm {                                                 
  protected:
    std::string password;
    VideoDecoder *dec;
  public:                                                                       
    virtual void embed(Chunk *c, char *data, int dataBytes, int offset) = 0;
    virtual void extract(Chunk *c, char *output, int dataBytes, int offset) = 0;                                 
    virtual void getAlgorithmCode(char out[4]) = 0;
};
#endif
