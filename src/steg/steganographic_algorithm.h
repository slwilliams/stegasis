#ifndef STEGALG_H
#define STEGALG_H 
class SteganographicAlgorithm {                                                 
  public:                                                                       
    virtual void embed(char *frame, char *data, int dataBytes, int offset) = 0;
    virtual void extract(char *frame, char *output, int dataBytes, int offset) = 0;                                 
};
#endif
