#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <math.h>

#include <gflags/gflags.h>

#include "common/progress_bar.h"

#include "fs/fswrapper.h"
#include "fs/stegfs.h"

#include "video/video_decoder.h" 
#include "video/avi_decoder.cc"
#include "video/mp4_decoder.cc"
#include "video/other_decoder.cc"

#include "steg/steganographic_algorithm.h"
#include "steg/lsb_algorithm.cc"
#include "steg/lsbp_algorithm.cc"
#include "steg/dctl_algorithm.cc"
#include "steg/dctp_algorithm.cc"
#include "steg/f4_algorithm.cc"
#include "steg/f5_algorithm.cc"

#include "crypt/cryptographic_algorithm.h"
#include "crypt/aes_algorithm.cc"
#include "crypt/twofish_algorithm.cc"
#include "crypt/serpent_algorithm.cc"
#include "crypt/aes_serpent_twofish_algorithm.cc"
#include "crypt/id_algorithm.cc"

using namespace std;

void printName();
void printUsage();
void incorrectArgNumber(string command);
void doFormat(string stegAlg, string cryptAlg, string pass, string pass2, int capacity, string videoPath);
void doMount(string videoPath, string mountPoint, string stegAlg, string cryptAlg, string pass, bool performance);
void writeHeader(SteganographicAlgorithm *alg, VideoDecoder *dec, char capacityB, int frame);
SteganographicAlgorithm *getSteg(string alg, string pass, VideoDecoder *dec, CryptographicAlgorithm *crpyt);
CryptographicAlgorithm *getCrypt(string alg, string pass, VideoDecoder *dec);

DEFINE_string(alg, "dctl", "Embedding algorithm to use");
DEFINE_string(crypt, "", "Encrypting algorithm to use");
DEFINE_string(pass, "", "Passphrase to encrypt and permute data with");
DEFINE_string(pass2, "", "Passphrase to encrypt and permute the hidden volume");
DEFINE_int32(cap, 20, "Percentage of frame to embed within");
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
    // stegasis format --alg=lsbp --crypt=aes --pass=123 --cap=10 /media/video.avi
    if (argc != 3) {
      incorrectArgNumber(command);
      return 1;
    }
    string videoPath = argv[2];
    doFormat(FLAGS_alg, FLAGS_crypt, FLAGS_pass, FLAGS_pass2, FLAGS_cap, videoPath);
  } else if (command == "mount") {
    // stegasis mount [-p,-f] --alb=lsbp --crypt=aes --pass=123 /media/video.avi /tmp/test
    if (argc != 4) {
      incorrectArgNumber(command);
      return 1;
    }
    string videoPath = argv[2];
    string mountPoint = argv[3];
    doMount(videoPath, mountPoint, FLAGS_alg, FLAGS_crypt, FLAGS_pass, FLAGS_p); 
  } else {
    printf("Unknown command: %s\n", command.c_str());
    return 1;
  }
  return 0;
}

void doMount(string videoPath, string mountPoint, string stegAlg, string cryptAlg, string pass, bool performance) {
  string extension = videoPath.substr(videoPath.find_last_of(".") + 1);
  VideoDecoder *dec = NULL;
  if (FLAGS_f) {
    dec = (VideoDecoder *)new JPEGDecoder(videoPath, false);
  } else {
    dec = extension == "avi" ? (VideoDecoder *)new AVIDecoder(videoPath) : (VideoDecoder *)new JPEGDecoder(videoPath, false);
  }
  CryptographicAlgorithm *crypt = getCrypt(cryptAlg, pass, dec);
  SteganographicAlgorithm *alg = getSteg(stegAlg, pass, dec, crypt); 
  SteganographicFileSystem::Set(new SteganographicFileSystem(dec, alg, performance)); 

  printf("\nMounting at %s...\n", mountPoint.c_str());
  wrap_mount(mountPoint);
  printf("\nUnmounting...\n");

  SteganographicFileSystem::Instance()->writeHeader();
  delete dec;

  printf("Sucsessfully unmounted!\n");
}

void doFormat(string stegAlg, string cryptAlg, string pass, string pass2, int capacity, string videoPath) {
  string extension = videoPath.substr(videoPath.find_last_of(".") + 1);
  VideoDecoder *dec = NULL; 
  //VideoDecoder *dec = (VideoDecoder *)new MP4Decoder(videoPath);
  if (FLAGS_f) {
    dec = (VideoDecoder *)new JPEGDecoder(videoPath, true);
  } else {
    dec = extension  == "avi" ? (VideoDecoder *)new AVIDecoder(videoPath) : (VideoDecoder *)new JPEGDecoder(videoPath, true);
  }
  //exit(0);
  CryptographicAlgorithm *crypt = getCrypt(cryptAlg, pass, dec);
  SteganographicAlgorithm *alg = getSteg(stegAlg, pass, dec, crypt);

  // Clamp capacity to 0-100
  capacity = max(0, capacity);
  capacity = min(capacity, 100);
  char capacityB = (char)capacity;
  
  if (pass2 != "") {
    // Hidden volume requested, write random data to every frame
    printf("Writing random data to video frames...\n");
    char *randomData = (char *)malloc(sizeof(char) * dec->getFrameSize()/8);
    for (int i = 0; i < dec->getFrameSize()/8; i ++) {
      randomData[i] = (char)rand();
    }
    for (int i = 0; i < dec->getNumberOfFrames(); i ++) {
      loadBar(i, dec->getNumberOfFrames() - 1, 50);
      alg->embed(dec->getFrame(i), randomData, dec->getFrameSize() / 8, 0);
    }
  }

  writeHeader(alg, dec, capacityB, 0);
  
  int totalCapacity = (int)floor((dec->getNumberOfFrames() * (dec->getFrameSize() / 8000) * (capacity / 100.0)));
  printf("\x1B[1;32mVolume capacity: %.2fMB\033[0m\n\n", totalCapacity/1000.0); 

  if (pass2 != "") {
    // Hidden volume requested
    CryptographicAlgorithm *crypt2 = getCrypt(cryptAlg, pass2, dec);
    SteganographicAlgorithm *alg2 = getSteg(stegAlg, pass2, dec, crypt2);
    writeHeader(alg2, dec, capacityB, dec->getNumberOfFrames() / 2);
  }

  delete dec;
  printf("Format successful!\n");
}

void writeHeader(SteganographicAlgorithm *alg, VideoDecoder *dec, char capacityB, int frame) {
  int currentFrame = frame, currentOffset = 0;
  int bytesWritten = 0;
  char header[4] = {'S', 'T', 'E', 'G'};
  do {
    pair<int, int> written = alg->embed(dec->getFrame(currentFrame), header+bytesWritten, 4-bytesWritten, currentOffset); 
    bytesWritten += written.first;
    currentOffset = written.second;
    if (written.second == 0) currentFrame ++;
  } while (bytesWritten != 4);
  bytesWritten = 0;

  char algCode[4];
  alg->getAlgorithmCode(algCode);
  do {
    pair<int, int> written = alg->embed(dec->getFrame(currentFrame), algCode+bytesWritten, 4-bytesWritten, currentOffset); 
    bytesWritten += written.first;
    currentOffset = written.second;
    if (written.second == 0) currentFrame ++;
  } while (bytesWritten != 4);
  bytesWritten = 0;

  do {
    pair<int, int> written = alg->embed(dec->getFrame(currentFrame), &capacityB+bytesWritten, 1-bytesWritten, currentOffset); 
    bytesWritten += written.first;
    currentOffset = written.second;
    if (written.second == 0) currentFrame ++;
  } while (bytesWritten != 1);
  bytesWritten = 0;

  int headerBytes = 0;
  do {
    pair<int, int> written = alg->embed(dec->getFrame(currentFrame), ((char *)&headerBytes)+bytesWritten, 4-bytesWritten, currentOffset); 
    bytesWritten += written.first;
    currentOffset = written.second;
    if (written.second == 0) currentFrame ++;
  } while (bytesWritten != 4);
}

SteganographicAlgorithm *getSteg(string alg, string pass, VideoDecoder *dec, CryptographicAlgorithm *crypt) {
  if (alg == "lsb") {
    return new LSBAlgorithm(pass, dec, crypt);
  } else if (alg == "lsbp") {
    return new LSBPAlgorithm(pass, dec, crypt);
  } else if (alg == "dctl") {
    return new DCTLAlgorithm(pass, dec, crypt);
  } else if (alg == "dctp") {
    return new DCTPAlgorithm(pass, dec, crypt);
  } else if (alg == "f4") {
    return new F4Algorithm(pass, dec, crypt);
  } else if (alg == "f5") {
    return new F5Algorithm(pass, dec, crypt);
  } else {
    printf("Unknown SteganographicAlgorithm: %s\n", alg.c_str());
    printUsage();
    exit(0);
  }
}

CryptographicAlgorithm *getCrypt(string alg, string pass, VideoDecoder *dec) {
  if (alg == "") {
    return new IDAlgorithm(pass, dec);
  } else if (alg == "aes") {
    return new AESAlgorithm(pass, dec);
  } else if (alg == "twofish") {
    return new TwofishAlgorithm(pass, dec);
  } else if (alg == "serpent") {
    return new SerpentAlgorithm(pass, dec);
  } else if (alg == "aes_serpent_twofish") {
    return new AESSerpentTwofishAlgorithm(pass, dec);
  } else {
    printf("Unknown CryptographicAlgorithm: %s\n", alg.c_str());
    printUsage();
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
  printf("                  __/ | v3.7a               \n");
  printf("                 |___/                      \n\n");
}

void printUsage() {
  printf("\nStegasis usage:\n");
  printf("  stegasis <command> [-p,-g] --alg=<alg> --pass=<pass> [--pass2=<pass2>] --cap=<capacity> <video_path> <mount_point>\n");
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
  printf("  --pass2  Passphrase used for encrypting and permuting the hidden volume\n");
  printf("  -p  Do not flush writes to disk until unmount\n");
  printf("  -f  Force the FFmpeg decoder to be used\n");
  printf("Embedding Algorithms:\n");
  printf("  Uncompressed AVI only:\n");
  printf("    lsb:  Least Significant Bit Sequential Embedding\n");
  printf("    lsbk:  LSB Sequential Embedding XOR'd with a psudo random stream\n");
  printf("    lsbp:  LSB Permuted Embedding using a seeded LCG\n");
  printf("    lsb2:  Combination of lsbk and lsbp\n");
  printf("    lsba:  LSB Permuted Embedding encrypted using AES\n");
  printf("  Other video formats:\n");
  printf("    dctl:  LSB Sequential Embedding within DCT coefficients\n");
  printf("    dctp:  LSB Permuted Embedding within DCT coefficients\n");
  printf("    dct2:  Combination of dctp and lsbk\n");
  printf("    dcta:  LSB Permuted Embedding encrypted with AES\n");
  printf("    dct3:  LSB Permuted Embedding encrypted with AES->Twofish->Serpent\n");
}
