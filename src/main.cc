#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fuse.h>

#include "fs/fswrapper.h"
#include "fs/stegfs.h"
#include "steg/steganographic_algorithm.h"
#include "video/video_decoder.h" 
#include "video/avi_decoder.cc"
#include "steg/lsb_algorithm.cc"
#include "steg/lsbk_algorithm.cc"

using namespace std;

void printName();
void printUsage();
void incorrectArgNumber(string command);
void doFormat(string algorithm, string videoPath);
void doMount(string videoPath, string mountPoint, bool performance);
SteganographicAlgorithm *getAlg(string alg);

int main(int argc, char *argv[]) {
  printName();
  if (argc < 2) {
    printf("Too few arguments.\n");
    printUsage();
    return 1;
  }

  string command = argv[1];
  if (command == "format") {
    if (argc != 4) {
      incorrectArgNumber(command);
      return 1;
    }
    string algFlag = argv[2];
    string videoPath = argv[3];
    string alg = algFlag.substr(algFlag.find("=") + 1, algFlag.length());

    doFormat(alg, videoPath);
  } else if (command == "mount") {
    if (argc < 4) {
      incorrectArgNumber(command);
      return 1;
    }
    string firstFlag = argv[2];
    string performanceFlag("-p");
    if (firstFlag == performanceFlag) {
      // Optional performance flag
      string videoPath = argv[3];
      string mountPoint = argv[4];
      doMount(videoPath, mountPoint, true); 
    } else {
      string videoPath = argv[2];
      string mountPoint = argv[3];
      doMount(videoPath, mountPoint, false);
    }
  } else if (command == "unmount") {
    if (argc != 3) {
      incorrectArgNumber(command);
      return 1;
    } 
    string prefix("fusermount -u ");
    string mountPoint(argv[2]);
    string command = prefix + mountPoint;
    printf("Unmounting %s\n", mountPoint.c_str());
    return system(command.c_str());
  } else {
    printf("Unknown command\n");
    return 1;
  }
  return 0;
}

void doMount(string videoPath, string mountPoint, bool performance) {
  SteganographicAlgorithm *lsb = new LSBKAlgorithm("LMBFTW");
  VideoDecoder *dec = new AVIDecoder(videoPath);
  SteganographicFileSystem::Set(new SteganographicFileSystem(dec, lsb, performance)); 

  printf("Mounting...\n");
  wrap_mount(mountPoint);

  printf("Writing video...\n");
  SteganographicFileSystem::Instance()->writeHeader();
  delete dec;

  printf("Sucsessfully unmounted\n");
}

void doFormat(string algorithm, string videoPath) {
  //SteganographicAlgorithm *alg = getAlg(algorithm);
  SteganographicAlgorithm *alg = new LSBKAlgorithm("LMBFTW");
  VideoDecoder *dec = new AVIDecoder(videoPath);
  char header[4] = {'S', 'T', 'E', 'G'};

  Chunk *headerFrame = dec->getFrame(0);
  alg->embed(headerFrame->getFrameData(), header, 4, 0);

  char algCode[4];
  alg->getAlgorithmCode(algCode);
  alg->embed(headerFrame->getFrameData(), algCode, 4, 4 * 8);

  union {
    uint32_t num;
    char byte[4];
  } headerBytes;
  // Hard coded for now
  headerBytes.num = 0;

  alg->embed(headerFrame->getFrameData(), headerBytes.byte, 4, 8 * 8);
  
  // Make sure the header is written back
  headerFrame->setDirty();

  delete dec;
  delete alg;
}

SteganographicAlgorithm *getAlg(string alg) {
  if (alg == "lsb") {
    return new LSBAlgorithm;
  }
}

void incorrectArgNumber(string command) {
  printf("Incorrect number of arguments for command %s\n", command.c_str());
  printUsage();
}

void printName() {
  printf("   _____ _                                  \n");
  printf("  / ____| |                     (_)         \n");
  printf(" | (___ | |_ ___  __ _  __ _ ___ _ ___      \n");
  printf("  \\___ \\| __/ _ \\/ _` |/ _` / __| / __|  \n");
  printf("  ____) | ||  __/ (_| | (_| \\__ \\ \\__ \\ \n");
  printf(" |_____/ \\__\\___|\\__, |\\__,_|___/_|___/ \n");
  printf("                  __/ | v0.1                \n");
  printf("                 |___/                      \n");
}

void printUsage() {
  printf("Stegasis usage:\n");
}
