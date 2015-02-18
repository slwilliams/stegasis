#ifndef viddec_h
#define viddec_h

class Frame {
  protected:
    long frameSize;
  public:
    virtual long getFrameSize() = 0;
    virtual char *getFrameData(int n=0, int c=0) = 0;
    virtual bool isDirty() = 0;
    virtual void setDirty() = 0;
};

class VideoDecoder {
  public:
    virtual Frame *getFrame(int frame) = 0;
    virtual Frame *getHeaderFrame() = 0;

    virtual int getFileSize() = 0;
    virtual int getNumberOfFrames() = 0;
    virtual int getFrameSize() = 0;
    virtual int getFrameHeight() = 0;
    virtual int getFrameWidth() = 0;
    virtual int getCapacity() = 0;
    virtual void getNextFrameOffset(int *frame, int *offset) = 0;

    virtual void setNextFrameOffset(int frame, int offset) = 0;
    virtual void setCapacity(char capacity) = 0;
    virtual void setHiddenVolume() = 0;

    virtual void writeBack() = 0;
    virtual ~VideoDecoder() {};
};

#endif
