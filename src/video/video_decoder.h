#ifndef VIDDEC_H
#define VIDDEC_H
class VideoDecoder {
  public:
    virtual char **getFrame(int frame) = 0;
    virtual bool writeFrame(int frame, char frameData[]) = 0;
    virtual int fileSize() = 0;
    virtual int numberOfFrames() = 0;
};
#endif
