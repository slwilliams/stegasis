#ifndef STEGALG_H
#define STEGALG_H 
class SteganographicAlgorithm {                                                 
  public:                                                                       
    virtual void embed(char *frame[], char data[]) = 0;
    virtual char **extract(char *frame[]) = 0;                                 
};
#endif
