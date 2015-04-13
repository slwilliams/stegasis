#include <utility>
#include <string>

#include "video/video_decoder.h"
#include "steg/steganographic_algorithm.h"
#include "crypt/cryptographic_algorithm.h"

#include "steg/steganographic_algorithm.h"
#include "steg/lsb_algorithm.cc"
#include "steg/lsbp_algorithm.cc"
#include "steg/dctl_algorithm.cc"
#include "steg/dctp_algorithm.cc"
#include "steg/f4_algorithm.cc"
#include "steg/f5_algorithm.cc"

#include "crypt/cryptographic_algorithm.h"
#include "crypt/aes_algorithm.cc"
#include "crypt/twofish_algorithm.cc"
#include "crypt/serpent_algorithm.cc"
#include "crypt/aes_serpent_twofish_algorithm.cc"
#include "crypt/id_algorithm.cc"

#define ASSERT(cond, text) if (!cond) { printf(text); abort(); }

using namespace std;

class MockFrame : public Frame {
  public:
    MockFrame(void *data): data(data) {};
    long getFrameSize() { return 0; };
    char *getFrameData(int n=0, int c=0) { return (char *)this->data; };
    bool isDirty() { return false; };
    void setDirty() {};

  private:
    void *data;
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

class StegAlgorithmTests {
  public:
    StegAlgorithmTests(SteganographicAlgorithm *alg, VideoDecoder *dec, string name): alg(alg), dec(dec), name(name) {};
    void runTests() {
      printf("Running tests for %s...\n", this->name.c_str());

      this->AlgorithmShouldReturnZeroBytesEmbeddedIfZeroBytesRequested();
      this->AlgorithmShouldCorrectlyExtractEmbededData();
      this->AlgorithmShouldNotModifyGivenData();

      printf("All tests complete for %s!\n", this->name.c_str());
      printf("---------------------------------\n\n");
    };
  private:
    SteganographicAlgorithm *alg;
    VideoDecoder *dec;
    string name;

    void AlgorithmShouldReturnZeroBytesEmbeddedIfZeroBytesRequested() {
      pair<int, int> output = this->alg->embed(this->dec->getFrame(0), NULL, 0, 0);
      ASSERT(output.first == 0, "StegAlgorithm did not return zero bytes when requested\n")
    };

    void AlgorithmShouldCorrectlyExtractEmbededData() {
      char *randomData = (char *)malloc(sizeof(char) * 10);
      for (int i = 0; i < 10; i ++) { randomData[i] = rand(); }
      pair<int, int> output = this->alg->embed(this->dec->getFrame(0), randomData, 10, 0);
      ASSERT(output.first == 10, "StegAlgorithm did not embed 10 bytes as requested\n")
      char *outputData = (char *)malloc(sizeof(char) * 10);
      this->alg->extract(this->dec->getFrame(0), outputData, 10, 0);
      for (int i = 0; i < 10; i ++) {
        ASSERT(randomData[i] == outputData[i], "StegAlgorithm extracted data did not match requested embedded data\n")
      }
    };

    void AlgorithmShouldNotModifyGivenData() {
      char *randomData = (char *)malloc(sizeof(char) * 10);
      for (int i = 0; i < 10; i ++) { randomData[i] = rand(); }
      char *copy = (char *)malloc(sizeof(char) * 10);
      memcpy(copy, randomData, 10);
      pair<int, int> output = this->alg->embed(this->dec->getFrame(0), randomData, 10, 0);
      ASSERT(output.first == 10, "StegAlgorithm did not embed 10 bytes as requested\n")
      for (int i = 0; i < 10; i ++) {
        ASSERT(randomData[i] == copy[i], "StegAlgorithm incorrectly modified given data\n")
      }
    };
};


int main(int argc, char **argv) {
  char *mockData = (char *)calloc(100, sizeof(char));
  MockFrame *mockFrame = new MockFrame(mockData);
  MockVideoDecoder *mockDecoder = new MockVideoDecoder(mockFrame, 100);
  LSBAlgorithm *lsbAlg = new LSBAlgorithm("123", mockDecoder, new IDAlgorithm("123", mockDecoder)); 
  StegAlgorithmTests *lsb_test = new StegAlgorithmTests(lsbAlg, mockDecoder, "LSBAlgorithm");
  lsb_test->runTests();

  LSBPAlgorithm *lsbpAlg = new LSBPAlgorithm("123", mockDecoder, new IDAlgorithm("123", mockDecoder)); 
  StegAlgorithmTests *lsbp_test = new StegAlgorithmTests(lsbpAlg, mockDecoder, "LSBPAlgorithm");
  lsbp_test->runTests();

  LSBPAlgorithm *lsbpAESAlg = new LSBPAlgorithm("123", mockDecoder, new AESAlgorithm("123", mockDecoder)); 
  StegAlgorithmTests *lsbpAES_test = new StegAlgorithmTests(lsbpAESAlg, mockDecoder, "LSBPAlgorithm_AES");
  lsbpAES_test->runTests();

  LSBPAlgorithm *lsbpTwofishAlg = new LSBPAlgorithm("123", mockDecoder, new TwofishAlgorithm("123", mockDecoder)); 
  StegAlgorithmTests *lsbpTwofish_test = new StegAlgorithmTests(lsbpTwofishAlg, mockDecoder, "LSBPAlgorithm_Twofish");
  lsbpTwofish_test->runTests();

  LSBPAlgorithm *lsbpSerpentAlg = new LSBPAlgorithm("123", mockDecoder, new SerpentAlgorithm("123", mockDecoder)); 
  StegAlgorithmTests *lsbpSerpent_test = new StegAlgorithmTests(lsbpSerpentAlg, mockDecoder, "LSBPAlgorithm_Serpent");
  lsbpSerpent_test->runTests();

  LSBPAlgorithm *lsbpThreeAlg = new LSBPAlgorithm("123", mockDecoder, new AESSerpentTwofishAlgorithm("123", mockDecoder)); 
  StegAlgorithmTests *lsbpThree_test = new StegAlgorithmTests(lsbpThreeAlg, mockDecoder, "LSBPAlgorithm_Three");
  lsbpThree_test->runTests();
 
  // Tests for jpeg embedding algorithms
  // JBLOCKARRAY is a 3D array of shorts
  short ***jpegData = new short**[1];
  for (int i = 0; i < 1; i ++) {
    jpegData[i] = new short*[10];
    for (int j = 0; j < 10; j ++)
      jpegData[i][j] = new short[64];
  }

  MockFrame *mockJpegFrame = new MockFrame(jpegData);
  MockVideoDecoder *mockJpegDecoder = new MockVideoDecoder(mockJpegFrame, 100);

  DCTLAlgorithm *dctlAlg = new DCTLAlgorithm("123", mockJpegDecoder, new IDAlgorithm("123", mockJpegDecoder));
  StegAlgorithmTests *dctl_test = new StegAlgorithmTests(dctlAlg, mockJpegDecoder, "DCTLAlgorithm");
  dctl_test->runTests();

  DCTPAlgorithm *dctpAlg = new DCTPAlgorithm("123", mockJpegDecoder, new IDAlgorithm("123", mockJpegDecoder));
  StegAlgorithmTests *dctp_test = new StegAlgorithmTests(dctpAlg, mockJpegDecoder, "DCTPAlgorithm");
  dctp_test->runTests();

  F4Algorithm *f4Alg = new F4Algorithm("123", mockJpegDecoder, new IDAlgorithm("123", mockJpegDecoder));
  StegAlgorithmTests *f4_test = new StegAlgorithmTests(f4Alg, mockJpegDecoder, "F4Algorithm");
  f4_test->runTests();

  F5Algorithm *f5Alg = new F5Algorithm("123", mockJpegDecoder, new IDAlgorithm("123", mockJpegDecoder));
  StegAlgorithmTests *f5_test = new StegAlgorithmTests(f4Alg, mockJpegDecoder, "F5Algorithm");
  f5_test->runTests();

  printf("All tests passed :)\n");
  return 0;
}
