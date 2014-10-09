#ifndef logging_h
#define logging_h

#include <string>
#include <stdio.h>

class Logger {
  private:
    FILE *output;
    bool useStdOut;
  public:
    Logger(std::string file, bool useStdOut);
    void log(std::string message);
};
#endif