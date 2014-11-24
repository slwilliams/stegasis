#ifndef viddec_h
#define viddec_h

class Chunk {
  protected:
    long chunkSize;
  public:
    virtual long getChunkSize() = 0;
    virtual char *getFrameData(int n=0, int c=0) = 0;
    virtual bool isDirty() = 0;
    virtual void setDirty() = 0;
    virtual void *getAdditionalData() {};
};

class VideoDecoder {
  public:
    virtual Chunk *getFrame(int frame) = 0;
    virtual int getFileSize() = 0;
    virtual int numberOfFrames() = 0;
    virtual void getNextFrameOffset(int *frame, int *offset) = 0;
    virtual void setNextFrameOffset(int frame, int offset) = 0;
    virtual int frameSize() = 0;
    virtual int frameHeight() = 0;
    virtual int frameWidth() = 0;
    virtual void writeBack() = 0;
    virtual void setCapacity(char capacity) = 0;
    virtual ~VideoDecoder() {};
};

#endif
