#include <stdio.h>

#include "video/avi_decoder.cc"
#include "video/video_decoder.h"

// ./main.o /media/vid.avi test.ppm 3 rgb
int main(int argc, char **argv) {
  string videoPath = argv[1];
  string outputFile = argv[2];
  int frame = atoi(argv[3]);
  string rgb = argv[4];
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

  if (rgb == "rgb") {
    char c = '2';
    fwrite((char *)&c, 1, 1, f);
    c = '5';
    fwrite((char *)&c, 1, 1, f);
    fwrite((char *)&c, 1, 1, f);
  } else {
    char c = '1';
    fwrite((char *)&c, 1, 1, f);
  }
  fwrite((char *)&space, 1, 1, f);
 
  dec->setCapacity(100);
  int frameSize = dec->frameSize();
  printf("framesize: %d\n", frameSize);
  Chunk *c = dec->getFrame(frame);
  int j = 0;
  for (j = 0; j < frameSize; j += 3) {
    char blue = c->getFrameData(0)[j];
    char green = c->getFrameData(0)[j+1];
    char red = c->getFrameData(0)[j+2];
    if (rgb == "rgb") {
      fwrite(&red, 1, 1, f);
      fwrite(&green, 1, 1, f);
      fwrite(&blue, 1, 1, f);
    } else {
      char blueLsb = blue & 1;
      char greenLsb = green & 1;
      char redLsb = red & 1;
      char toWrite;
      if (rgb == "r") {
        toWrite = redLsb;
      } else if (rgb == "g") {
        toWrite = greenLsb;
      } else if (rgb == "b") {
        toWrite = blueLsb;
      }
      fwrite(&toWrite, 1, 1, f);
      fwrite(&toWrite, 1, 1, f);
      fwrite(&toWrite, 1, 1, f);
    }
  }
  fclose(f);
}
