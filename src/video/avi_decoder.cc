#include <stdio.h>
#include <string>
#include "video_decoder.h"

using namespace std;

class AVIDecoder : public VideoDecoder {
  private:
    string filePath;
    FILE *f;
  public:
    AVIDecoder(string filePath): filePath(filePath) {
     this->f = fopen(this->filePath.c_str(), "w"); 
    };
    ~AVIDecoder() {
      fclose(this->f);
    };
    virtual char **getFrame(int frame) {
      
    };                                     
    virtual bool writeFrame(int frame, char frameData[]) {
    
    };                   
    virtual int fileSize() {
     
    };                                                 
    virtual int numberOfFrames() {
    
    };   
};
