#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fuse.h>
#include <set>

#include "fs/fswrapper.h"
#include "fs/stegfs.h"
#include "steg/steganographic_algorithm.h"
#include "video/video_decoder.h" 
#include "video/avi_decoder.cc"
#include "steg/lsb_algorithm.cc"
#include "steg/lsbk_algorithm.cc"
#include "steg/lsbp_algorithm.cc"
#include "steg/lsb2_algorithm.cc"

using namespace std;

void printName();
void printUsage();
void incorrectArgNumber(string command);
void doFormat(string algorithm, string pass, string videoPath, char capacity);
void doMount(string videoPath, string mountPoint, string alg, string pass, bool performance);
bool algRequiresPass(string alg);
SteganographicAlgorithm *getAlg(string alg, string pass, VideoDecoder *dec);

int main(int argc, char *argv[]) {
  printName();
  if (argc < 2) {
    printf("Error: Insufficient arguments specified.\n");
    printUsage();
    return 1;
  }

  string command = argv[1];
  if (command == "format") {
    // stegasis format --alg=lsbk --pass=123 /media/video.avi
    if (argc < 4) {
      incorrectArgNumber(command);
      return 1;
    }
    string algFlag = argv[2];
    string alg = algFlag.substr(algFlag.find("=") + 1, algFlag.length());
    if (algRequiresPass(alg)) {
      string passFlag = argv[3];
      string pass = passFlag.substr(passFlag.find("=") + 1, passFlag.length());
      string videoPath = argv[4];
      doFormat(alg, pass, videoPath, 50);
    } else {
      string videoPath = argv[3];
      doFormat(alg, "", videoPath, 50);
    }
  } else if (command == "mount") {
    // stegasis mount [-p] --alb=lsbk --pass=123 /media/video.avi /tmp/test
    if (argc < 5) {
      incorrectArgNumber(command);
      return 1;
    }
    string firstFlag = argv[2];
    string performanceFlag("-p");
    if (firstFlag == performanceFlag) {
      // Optional performance flag
      string algFlag = argv[3];
      string alg = algFlag.substr(algFlag.find("=") + 1, algFlag.length());
      if (algRequiresPass(alg)) {
        string passFlag = argv[4];
        string pass = passFlag.substr(passFlag.find("=") + 1, passFlag.length());
        string videoPath = argv[5];
        string mountPoint = argv[6];
        doMount(videoPath, mountPoint, alg, pass, true); 
      } else {
        string videoPath = argv[4];
        string mountPoint = argv[5];
        doMount(videoPath, mountPoint, alg, "", true); 
      }
    } else {
      string algFlag = argv[2];
      string alg = algFlag.substr(algFlag.find("=") + 1, algFlag.length());
      if (algRequiresPass(alg)) {
        string passFlag = argv[3];
        string pass = passFlag.substr(passFlag.find("=") + 1, passFlag.length());
        string videoPath = argv[4];
        string mountPoint = argv[5];
        doMount(videoPath, mountPoint, alg, pass, false);
      } else {
        string videoPath = argv[3];
        string mountPoint = argv[4];
        doMount(videoPath, mountPoint, alg, "", false);
      }
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

void doMount(string videoPath, string mountPoint, string alg, string pass, bool performance) {
  VideoDecoder *dec = new AVIDecoder(videoPath);
  SteganographicAlgorithm *lsb = getAlg(alg, pass, dec); 
  SteganographicFileSystem::Set(new SteganographicFileSystem(dec, lsb, performance)); 

  printf("Mounting...\n");
  wrap_mount(mountPoint);

  printf("Writing video...\n");
  SteganographicFileSystem::Instance()->writeHeader();
  delete dec;

  printf("Sucsessfully unmounted\n");
}

void doFormat(string algorithm, string pass, string videoPath, char capacity) {
  VideoDecoder *dec = new AVIDecoder(videoPath);
  SteganographicAlgorithm *alg = getAlg(algorithm, pass, dec);
  char header[4] = {'S', 'T', 'E', 'G'};

  Chunk *headerFrame = dec->getFrame(0);
  alg->embed(headerFrame->getFrameData(), header, 4, 0);

  char algCode[4];
  alg->getAlgorithmCode(algCode);
  alg->embed(headerFrame->getFrameData(), algCode, 4, 4 * 8);

  alg->embed(headerFrame->getFrameData(), &capacity, 1, 8 * 8);

  union {
    uint32_t num;
    char byte[4];
  } headerBytes;
  headerBytes.num = 0;

  alg->embed(headerFrame->getFrameData(), headerBytes.byte, 4, 9 * 8);
  
  // Make sure the header is written back
  headerFrame->setDirty();

  delete dec;
  delete alg;
}

bool algRequiresPass(string alg) {
  return alg == "lsbk" || alg == "lsbp" || alg == "lsb2";
}

SteganographicAlgorithm *getAlg(string alg, string pass, VideoDecoder *dec) {
  if (alg == "lsb") {
    return new LSBAlgorithm;
  } else if (alg == "lsbk") {
    return new LSBKAlgorithm(pass, dec);
  } else if (alg == "lsbp") {
    return new LSBPAlgorithm(pass, dec);
  } else if (alg == "lsb2") {
    return new LSB2Algorithm(pass, dec);
  } else {
    printf("Unknown algorithm\n");
    exit(0);
  }
}

void incorrectArgNumber(string command) {
  printf("Error: Incorrect number of arguments for command '%s'\n", command.c_str());
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
  printf("\nStegasis usage:\n");
  printf("---------------------------\n");
  printf("Example commands:\n");
  printf("\tstegasis format --alg=lsbk --pass=password123 /media/video.avi\n");
  printf("\tstegasis mount --alg=lsbk --password123 /media/video.avi /tmp/test\n"); 
  printf("Optional flags:\n");
  printf("\t-p Do not flush writes to disk until unmount\n");
  printf("\t-g Use green channel for embedding\n");
}
