#ifndef lcg_h
#define lcg_h

#include <set>

class LCG {
  private:
    int m, c;
    int trueM;
    int a;
    int seed;
    int gcd(int u, int v);
  public:
    LCG(int m);
    void setSeed(int seed);
    int iterate();
    void debug();
};
#endif
