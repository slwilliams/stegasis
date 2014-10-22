#ifndef viddec_h
#define viddec_h

class Chunk {
  public:
    virtual int getChunkSize() = 0;
    virtual char *getFrameData() = 0;
    virtual bool isDirty() = 0;
    virtual void setDirty() = 0;
};

class VideoDecoder {
  public:
    virtual Chunk *getFrame(int frame) = 0;
    virtual int getFileSize() = 0;
    virtual int numberOfFrames() = 0;
    virtual void getNextFrameOffset(int *frame, int *offset) = 0;
    virtual void setNextFrameOffset(int frame, int offset) = 0;
    virtual int frameSize() = 0;
    virtual void writeBack() = 0;
    virtual ~VideoDecoder() {};
};

#endif
