#include <stdio.h>

#include "video/avi_decoder.cc"
#include "video/video_decoder.h"

int main(int argc, char **argv) {
  string videoPath = argv[1];
  string outputFile = argv[2];
  int frame = atoi(argv[3]);
  FILE *f = fopen(outputFile.c_str(), "wb");
  AVIDecoder *dec = new AVIDecoder(videoPath);
  
  // Write the image size and stuff
  char header[2] = {'P', '6'};
  fwrite((char *)header, 1, 2, f);

  char space = ' ';
  fwrite((char *)&space, 1, 1, f);

  int width = dec->frameWidth();
  char *tempAscii = (char *)malloc(10 * sizeof(char));
  int length = sprintf(tempAscii, "%d", width);
  fwrite(tempAscii, 1, length, f);
  fwrite((char *)&space, 1, 1, f);

  int height = dec->frameHeight();
  length = sprintf(tempAscii, "%d", height);
  fwrite(tempAscii, 1, length, f);
  fwrite((char *)&space, 1, 1, f);

  char one = '2';
  fwrite((char *)&one, 1, 1, f);
  one = '5';
  fwrite((char *)&one, 1, 1, f);
  fwrite((char *)&one, 1, 1, f);
  fwrite((char *)&space, 1, 1, f);
 
  int frameSize = dec->frameSize();
  Chunk *c = dec->getFrame(frame);
  int j = 0;

  for (j = 0; j < frameSize; j += 3) {
    char blue = c->getFrameData()[j];
    char green = c->getFrameData()[j+1];
    char red = c->getFrameData()[j+2];
    fwrite(&red, 1, 1, f);
    fwrite(&green, 1, 1, f);
    fwrite(&blue, 1, 1, f);
    char blueLsb = blue & 1;
    char greenLsb = green & 1;
    char redLsb = red & 1;
    //fwrite(&redLsb, 1, 1, f);
    //fwrite(&redLsb, 1, 1, f);
    //fwrite(&redLsb, 1, 1, f);
    /*fwrite(&greenLsb, 1, 1, f);
    fwrite(&greenLsb, 1, 1, f);
    fwrite(&greenLsb, 1, 1, f);
    fwrite(&blueLsb, 1, 1, f);
    fwrite(&blueLsb, 1, 1, f);
    fwrite(&blueLsb, 1, 1, f);*/
  }
  fclose(f);
}