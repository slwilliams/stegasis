#include <string>
#include <stdio.h>
#include "common/logging.h"

using namespace std;

Logger::Logger(string file, bool useStdOut): useStdOut(useStdOut) {
  this->output = fopen(file.c_str(), "w");
};

void Logger::log(string message) {
  if (this->useStdOut) {
    printf("%s", message.c_str());
  } else {
   fputs(message.c_str(), this->output); 
   fflush(this->output);
  }
};
