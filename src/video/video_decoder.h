#ifndef viddec_h
#define viddec_h

class Chunk {
  public:
    virtual int getChunkSize() = 0;
    virtual char *getFrameData() = 0;
    virtual bool isDirty() = 0;
};

class VideoDecoder {
  public:
    virtual Chunk *getFrame(int frame) = 0;
    virtual int getFileSize() = 0;
    virtual int numberOfFrames() = 0;
    virtual ~VideoDecoder() {};
};

#endif
