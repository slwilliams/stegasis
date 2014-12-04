#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include <gflags/gflags.h>

#include "fs/fswrapper.h"
#include "fs/stegfs.h"

#include "video/video_decoder.h" 
#include "video/avi_decoder.cc"
#include "video/other_decoder.cc"

#include "steg/steganographic_algorithm.h"
#include "steg/lsb_algorithm.cc"
#include "steg/lsbk_algorithm.cc"
#include "steg/lsbp_algorithm.cc"
#include "steg/lsb2_algorithm.cc"
#include "steg/dctl_algorithm.cc"
#include "steg/dctp_algorithm.cc"
#include "steg/dct2_algorithm.cc"
#include "steg/dctp_aes_algorithm.cc"
#include "steg/dctp_aes_tfish_serpent_algorithm.cc"

using namespace std;

void printName();
void printUsage();
void incorrectArgNumber(string command);
void doFormat(string algorithm, string pass, int capacity, string videoPath);
void doMount(string videoPath, string mountPoint, string alg, string pass, bool performance);
SteganographicAlgorithm *getAlg(string alg, string pass, VideoDecoder *dec);

DEFINE_string(alg, "", "Embedding algorithm to use");
DEFINE_string(pass, "", "Passphrase to encrypt and permute data with");
DEFINE_int32(cap, 100, "Percentage of frame to embed within");
DEFINE_bool(p, false, "Do not write back to disk until unmount");
DEFINE_bool(f, false, "Force FFmpeg decoder to be used");

int main(int argc, char *argv[]) {
  // After parsing the flags, argv[1] will the command (format, mount)
  // argv[2] and argv[3] will be the video and mount point  
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  printName();

  if (argc < 2) {
    printUsage();
    return 1;
  }

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
  VideoDecoder *dec = NULL;
  if (FLAGS_f) {
    dec = (VideoDecoder *)new JPEGDecoder(videoPath, false);
  } else {
    dec = extension  == "avi" ? (VideoDecoder *)new AVIDecoder(videoPath) : (VideoDecoder *)new JPEGDecoder(videoPath, false);
  }
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
  VideoDecoder *dec = NULL;
  if (FLAGS_f) {
    dec = (VideoDecoder *)new JPEGDecoder(videoPath, true);
  } else {
    dec = extension  == "avi" ? (VideoDecoder *)new AVIDecoder(videoPath) : (VideoDecoder *)new JPEGDecoder(videoPath, true);
  }
  SteganographicAlgorithm *alg = getAlg(algorithm, pass, dec);
  
  Chunk *headerFrame = dec->getFrame(0);
  char header[4] = {'S', 'T', 'E', 'G'};
  alg->embed(headerFrame, header, 4, 0);

  char algCode[4];
  alg->getAlgorithmCode(algCode);
  alg->embed(headerFrame, algCode, 4, 4 * 8);

  if (capacity < 0) capacity = 0;
  if (capacity > 100) capacity = 100;
  char capacityB = (char)capacity;
  alg->embed(headerFrame, &capacityB, 1, 8 * 8);

  int headerBytes = 0;
  alg->embed(headerFrame, (char *)&headerBytes, 4, 9 * 8);
  
  // Make sure the header is written back
  headerFrame->setDirty();

  int totalCapacity = (int)floor((dec->numberOfFrames() * (dec->frameSize() / 8000) * (capacity / 100.0)));
  printf("Volume capacity: %.2fMB\n", totalCapacity/1000.0); 

  delete dec;
  printf("Format successful!\n");
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
  } else if (alg == "dct3") {
    return new DCT3Algorithm(pass, dec);
  } else if (alg == "dcta") {
    return new DCTAAlgorithm(pass, dec);
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
  printf("  stegasis <command> [-p,-g] --alg=<alg> --pass=<pass> --cap=<capacity> <video_path> <mount_point>\n");
  printf("-----------------------------------------------------------------\n");
  printf("Example useage:\n");
  printf("  stegasis format --alg=lsbk --pass=password123 --cap=50 /media/video.avi\n");
  printf("  stegasis mount --alg=lsbk --pass=password123 /media/video.avi /tmp/test\n"); 
  printf("Commands:\n");
  printf("  format  Formats a video for use with stegasis\n");
  printf("  mount  Mounts a formatted video to a given mount point\n");
  printf("Required Flags:\n");
  printf("  --alg  Embedding algorithm to use, see below\n");
  printf("  --cap  Percentage of frame to embed within in percent\n");
  printf("Optional flags:\n");
  printf("  --pass  Passphrase used for encrypting and permuting data\n");
  printf("  -p  Do not flush writes to disk until unmount\n");
  printf("  -g  Use green channel for embedding\n");
  printf("Embedding Algorithms:\n");
  printf("  Uncompressed AVI only:\n");
  printf("    lsb:  Least Significant Bit Sequential Embedding\n");
  printf("    lsbk:  LSB Sequential Embedding XOR'd with a psudo random stream\n");
  printf("    lsbp:  LSB Permuted Embedding using a seeded LCG\n");
  printf("    lsb2:  Combination of lsbk and lsbp\n");
  printf("  Other video formats:\n");
  printf("    dctl:  LSB Sequential Embedding within DCT coefficients\n");
  printf("    dctp:  LSB Permuted Embedding within DCT coefficients\n");
  printf("    dct2:  Combination of dctl and dctp\n");
  printf("    dcta:  LSB Permuted Embedding encrypted with AES\n");
  printf("    dct3:  LSB Permuted Embedding encrypted with AES->Twofish->Serpent\n");
}
