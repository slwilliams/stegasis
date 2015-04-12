#include <utility>
#include <string>

#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"
#include "crypt/cryptographic_algorithm.h"

#include "steg/lsb_algorithm.cc"
#include "crypt/id_algorithm.cc"

#define ASSERT(cond, text) if (!cond) { printf(text); abort(); }

using namespace std;

class MockFrame : public Frame {
  public:
    MockFrame(char *data): data(data) {};
    long getFrameSize() { return 0; };
    char *getFrameData(int n=0, int c=0) { return this->data; };
    bool isDirty() { return false; };
    void setDirty() {};

  private:
    char *data;
};


class MockVideoDecoder : public VideoDecoder {
  public:
    MockVideoDecoder(Frame *f, int cap): f(f), capacity(cap) {};
    Frame *getFrame(int frame) { return this->f; };
    Frame *getHeaderFrame() {return this->f; };
    int getFileSize() { return 0; };
    int getNumberOfFrames() { return 100; };
    int getFrameSize() { return 100; };
    int getFrameHeight() { return 10; };
    int getFrameWidth() { return 10; };
    int getCapacity() { return 100; };
    void getNextFrameOffset(int *f, int *o) {};
    void setNextFrameOffset(int f, int o) {};
    void setHiddenVolume() {};
    void setCapacity(char capacity) {};
    void writeBack() {};
  private:
    Frame *f;
    int capacity;
};


class LSBAlgorithmTests {
  public:
    void runTests() {
      this->LSBAlgorithmShouldReturnZeroBytesEmbeddedIfZeroBytesRequested();
    };
  private:
    void LSBAlgorithmShouldReturnZeroBytesEmbeddedIfZeroBytesRequested() {
      char *mockData = (char *)calloc(100, sizeof(char));
      Frame *mockFrame = new MockFrame(mockData);
      MockVideoDecoder *mockDecoder = new MockVideoDecoder(mockFrame, 100);
      LSBAlgorithm *alg = new LSBAlgorithm("", mockDecoder, new IDAlgorithm("", mockDecoder)); 
      pair<int, int> output = alg->embed(mockFrame, mockData, 0, 0);
      ASSERT(output.first == 0, "LSBAlgorithm did not return zero bytes when requested")
    };
};


int main(int argc, char **argv) {
  LSBAlgorithmTests lsb_test;
  lsb_test.runTests();

  printf("All tests passed :)\n");
  return 0;
}
