#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include <gflags/gflags.h>

#include "fs/fswrapper.h"
#include "fs/stegfs.h"
#include "steg/steganographic_algorithm.h"
#include "video/video_decoder.h" 
#include "video/avi_decoder.cc"
#include "video/other_decoder.cc"
#include "steg/lsb_algorithm.cc"
#include "steg/lsbk_algorithm.cc"
#include "steg/lsbp_algorithm.cc"
#include "steg/lsb2_algorithm.cc"
#include "steg/dctl_algorithm.cc"
#include "steg/dctp_algorithm.cc"
#include "steg/dct2_algorithm.cc"

using namespace std;

void printName();
void printUsage();
void incorrectArgNumber(string command);
void doFormat(string algorithm, string pass, int capacity, string videoPath);
void doMount(string videoPath, string mountPoint, string alg, string pass, bool performance);
bool algRequiresPass(string alg);
SteganographicAlgorithm *getAlg(string alg, string pass, VideoDecoder *dec);

DEFINE_string(alg, "", "Embedding algorithm to use");
DEFINE_string(pass, "", "Passphrase to encrypt and permute data with");
DEFINE_int32(cap, 100, "Percentage of frame to embed within");
DEFINE_bool(p, false, "Do not write back to disk until unmount");

int main(int argc, char *argv[]) {
  // After parsing the flags, argv[1] will the command (format, mount)
  // argv[2] and argv[3] will be the video and mount point  
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  printName();

  string command = argv[1];
  if (command == "format") {
    // stegasis format --alg=lsbk --pass=123 --cap=10 /media/video.avi
    if (argc != 3) {
      incorrectArgNumber(command);
      return 1;
    }
    string videoPath = argv[2];
    doFormat(FLAGS_alg, FLAGS_pass, FLAGS_cap, videoPath);
  } else if (command == "mount") {
    // stegasis mount [-p] --alb=lsbk --pass=123 /media/video.avi /tmp/test
    if (argc != 4) {
      incorrectArgNumber(command);
      return 1;
    }
    string videoPath = argv[2];
    string mountPoint = argv[3];
    doMount(videoPath, mountPoint, FLAGS_alg, FLAGS_pass, FLAGS_p); 
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
  string extension = videoPath.substr(videoPath.find_last_of(".") + 1);
  VideoDecoder *dec = extension  == "avi" ? (VideoDecoder *)new AVIDecoder(videoPath): (VideoDecoder *)new JPEGDecoder(videoPath, false);
  SteganographicAlgorithm *lsb = getAlg(alg, pass, dec); 
  SteganographicFileSystem::Set(new SteganographicFileSystem(dec, lsb, performance)); 

  printf("Mounting...\n");
  wrap_mount(mountPoint);
  printf("Unmounting...\n");

  SteganographicFileSystem::Instance()->writeHeader();
  delete dec;

  printf("Sucsessfully unmounted\n");
}

void doFormat(string algorithm, string pass, int capacity, string videoPath) {
  string extension = videoPath.substr(videoPath.find_last_of(".") + 1);
  VideoDecoder *dec = extension  == "avi" ? (VideoDecoder *)new AVIDecoder(videoPath): (VideoDecoder *)new JPEGDecoder(videoPath, true);
  SteganographicAlgorithm *alg = getAlg(algorithm, pass, dec);
  
  char header[4] = {'S', 'T', 'E', 'G'};

  Chunk *headerFrame = dec->getFrame(0);
  alg->embed(headerFrame, header, 4, 0);

  char algCode[4];
  alg->getAlgorithmCode(algCode);
  alg->embed(headerFrame, algCode, 4, 4 * 8);

  char capacityB = (char)capacity;
  alg->embed(headerFrame, &capacityB, 1, 8 * 8);

  int headerBytes = 0;
  alg->embed(headerFrame, (char *)&headerBytes, 4, 9 * 8);
  
  // Make sure the header is written back
  headerFrame->setDirty();

  int totalCapacity = (int)floor((dec->numberOfFrames() * (dec->frameSize() / 8000) * (capacityB / 100.0)));
  printf("Volume capacity: %.2fMB\n", totalCapacity/1000.0); 

  delete dec;
  printf("Format successful!\n");
}

bool algRequiresPass(string alg) {
  return alg == "lsbk" || alg == "lsbp" || alg == "lsb2" || alg == "dctp" || alg == "dct2";
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
  } else if (alg == "dctl") {
    return new DCTLAlgorithm(dec);
  } else if (alg == "dctp") {
    return new DCTPAlgorithm(pass, dec);
  } else if (alg == "dct2") {
    return new DCT2Algorithm(pass, dec);
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
  printf("                  __/ | v2.0a               \n");
  printf("                 |___/                      \n");
}

void printUsage() {
  printf("\nStegasis usage:\n");
  printf("---------------------------\n");
  printf("Example commands:\n");
  printf("\tstegasis format --alg=lsbk --pass=password123 --cap=50 /media/video.avi\n");
  printf("\tstegasis mount --alg=lsbk --pass=password123 /media/video.avi /tmp/test\n"); 
  printf("Required Flags:\n");
  printf("\t--alg  Embedding algorithm to use, one of [lsb, lsbk, lsbp, lsb2, dctl, dctp]\n");
  printf("\t--cap  Percentage of frame to embed within in percent\n");
  printf("Optional flags:\n");
  printf("\t--pass  Passphrase used for encrypting and permuting data, required for [lsbk, lsbp, lsb2, dctp]\n");
  printf("\t-p  Do not flush writes to disk until unmount\n");
  printf("\t-g  Use green channel for embedding\n");
}
