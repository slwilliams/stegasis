#include <stdio.h>
#include <unordered_map>
#include <string>

#include "video/other_decoder.cc"
#include "video/video_decoder.h"

// ./main.o output.mkv his.txt 3 1
int main(int argc, char **argv) {
  string videoPath = argv[1];
  string outputFile = argv[2];
  int frame = atoi(argv[3]);
  int comp = atoi(argv[4]);
  FILE *f = fopen(outputFile.c_str(), "wb");
  JPEGDecoder *dec = new JPEGDecoder(videoPath, false);
  Chunk *c = dec->getFrame(frame);

  unordered_map<int, int> freq = unordered_map<int, int>();
  JBLOCKARRAY frameData;
  for (int i = 0; i < dec->frameHeight(); i ++) {
    frameData = (JBLOCKARRAY)c->getFrameData(i, comp);
    for (int j = 0; j < dec->frameWidth(); j ++) {
      for (int k = 0; k < 64; k ++) {
        freq[frameData[0][j][k]] ++; 
      }
    }
  }

  char colon = ' ';
  char newLine = '\n';
  for (auto p : freq) {
    string c = std::to_string(p.first);
    string v = std::to_string(p.second);
    fwrite(c.c_str(), c.length(), 1, f);
    fwrite(&colon, 1, 1, f);
    fwrite(v.c_str(), v.length(), 1, f);
    fwrite(&newLine, 1, 1, f);
  }
  fclose(f);
}
