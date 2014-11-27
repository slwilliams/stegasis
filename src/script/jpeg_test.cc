#include <stdio.h>
#include <string>

#include "video/other_decoder.cc"
#include "video/video_decoder.h"

// ./test.o output.mkv 3 1 <co> <num>
int main(int argc, char **argv) {
  string videoPath = argv[1];
  int frame = atoi(argv[2]);
  int comp = atoi(argv[3]);
  int co = atoi(argv[4]);
  int num = atoi(argv[5]);
  JPEGDecoder *dec = new JPEGDecoder(videoPath, false);
  Chunk *c = dec->getFrame(frame);

  JBLOCKARRAY frameData;
  for (int i = 0; i < dec->frameHeight(); i ++) {
    frameData = (JBLOCKARRAY)c->getFrameData(i, comp);
    for (int j = 0; j < dec->frameWidth(); j ++) {
      for (int k = co; k < co + num; k ++) {
        frameData[0][j][k] ^= 1;
      }
    }
  }

  delete dec;
}
