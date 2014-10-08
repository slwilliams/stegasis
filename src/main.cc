#include <stdio.h>
#include <string>

#include "steg/steganographic_algorithm.h"
#include "video/video_decoder.h" 
#include "video/avi_decoder.cc"
#include "steg/lsb_algorithm.cc"
#include "fs/fuse.cc"

using namespace std;

void printName();
void printUsage();
void incorrectArgNumber(string command);
void doFormat(string algorithm, string videoPath);
void doMount(string videoPath, string mountPoint);

int main(int argc, char *argv[]) {
  SteganographicAlgorithm *lsb = new LSBAlgorithm;
  VideoDecoder *dec = new AVIDecoder("video.avi");
  SteganographicFileSystem *fs = new SteganographicFileSystem(dec, lsb);
  fs->mount("/tmp/test1");
  return 0;
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
    return 0;
  } else if (command == "mount") {
    if (argc != 4) {
      incorrectArgNumber(command);
      return 1;
    }
    string videoPath = argv[2];
    string mountPoint = argv[3];
    doMount(videoPath, mountPoint);
    return 0;
  } else {
    printf("Unknown command\n");
    return 1;
  }
}

void doFormat(string algorithm, string videoPath) {
  printf("alg: %s, path: %s\n", algorithm.c_str(), videoPath.c_str());		
}

void doMount(string videoPath, string mountPoint) {
  printf("videopath: %s, mountpoint: %s\n", videoPath.c_str(), 
      mountPoint.c_str());		
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
