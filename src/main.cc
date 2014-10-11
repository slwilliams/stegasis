#include <stdio.h>
#include <string>
#include <fuse.h>

#include "fs/fswrapper.h"
#include "fs/stegfs.h"
#include "steg/steganographic_algorithm.h"
#include "video/video_decoder.h" 
#include "video/avi_decoder.cc"
#include "steg/lsb_algorithm.cc"

using namespace std;

void printName();
void printUsage();
void incorrectArgNumber(string command);
void doFormat(string algorithm, string videoPath);

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
    if (argc != 4) {
      incorrectArgNumber(command);
      return 1;
    }
    string videoPath = argv[2];
    string mountPoint = argv[3];

    SteganographicAlgorithm *lsb = new LSBAlgorithm;
    VideoDecoder *dec = new AVIDecoder(videoPath);
    SteganographicFileSystem::Set(new SteganographicFileSystem(dec, lsb)); 

    wrap_mount(mountPoint);
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

void doFormat(string algorithm, string videoPath) {
  printf("alg: %s, path: %s\n", algorithm.c_str(), videoPath.c_str());		
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
